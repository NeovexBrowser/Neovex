// Copyright 2026 Neovex Authors
// Background update checker implementation.

#include "chrome/browser/neovex_update_checker.h"

#include <optional>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/process/launch.h"
#include "base/strings/string_util.h"
#include "base/task/thread_pool.h"
#include "base/version.h"
#include "chrome/browser/neovex_version.h"
#include "components/prefs/pref_service.h"
#include "net/base/load_flags.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

#if BUILDFLAG(IS_WIN)
#include <shlobj.h>
#include "base/win/windows_types.h"
#endif

namespace neovex {

const char kUpdateDownloaded[] = "neovex.update.downloaded";
const char kUpdateInstallerPath[] = "neovex.update.installer_path";
const char kUpdateLastVersion[] = "neovex.update.last_version";

namespace {

constexpr net::NetworkTrafficAnnotationTag kTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("neovex_update_check", R"(
      semantics {
        sender: "Neovex Update Checker"
        description:
          "Checks the GitHub Releases API for a newer version of Neovex "
          "and downloads the installer if one is available."
        trigger: "Browser startup."
        data: "No user data is sent. A simple GET request is made."
        destination: OTHER
      }
      policy {
        cookies_allowed: NO
        setting: "Cannot be disabled."
      })");

base::FilePath GetUpdatesDir() {
#if BUILDFLAG(IS_WIN)
  base::FilePath local_app_data;
  if (!base::PathService::Get(base::DIR_LOCAL_APP_DATA, &local_app_data)) {
    return base::FilePath();
  }
  return local_app_data.Append(L"Neovex").Append(L"Updates");
#else
  return base::FilePath();
#endif
}

// Compare two dotted version strings (e.g. "1.0.0" < "1.1.0").
bool IsNewerVersion(const std::string& remote_tag) {
  // Strip leading 'v' if present.
  std::string remote = remote_tag;
  if (!remote.empty() && remote[0] == 'v') {
    remote = remote.substr(1);
  }

  base::Version current(NEOVEX_VERSION);
  base::Version latest(remote);

  if (!current.IsValid() || !latest.IsValid()) {
    return false;
  }

  return latest > current;
}

}  // namespace

UpdateChecker::UpdateChecker(
    PrefService* local_state,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : local_state_(local_state),
      url_loader_factory_(std::move(url_loader_factory)) {}

UpdateChecker::~UpdateChecker() = default;

void UpdateChecker::CheckForUpdate() {
  // If we already downloaded an update, don't check again.
  if (local_state_->GetBoolean(kUpdateDownloaded)) {
    return;
  }

  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GURL(NEOVEX_GITHUB_RELEASES_API);
  request->load_flags =
      net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE;
  request->headers.SetHeader("Accept", "application/vnd.github.v3+json");
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;

  loader_ = network::SimpleURLLoader::Create(std::move(request),
                                             kTrafficAnnotation);
  loader_->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&UpdateChecker::OnReleaseFetched,
                     weak_factory_.GetWeakPtr()),
      512 * 1024);  // 512 KB max for the JSON response.
}

void UpdateChecker::OnReleaseFetched(std::optional<std::string> body) {
  if (!body) {
    LOG(WARNING) << "[Neovex] Update check failed: no response.";
    return;
  }

  auto parsed = base::JSONReader::Read(*body, base::JSON_PARSE_RFC);
  if (!parsed || !parsed->is_dict()) {
    LOG(WARNING) << "[Neovex] Update check failed: invalid JSON.";
    return;
  }

  const base::DictValue& dict = parsed->GetDict();
  const std::string* tag = dict.FindString("tag_name");
  if (!tag) {
    LOG(WARNING) << "[Neovex] Update check: no tag_name in release.";
    return;
  }

  if (!IsNewerVersion(*tag)) {
    VLOG(1) << "[Neovex] Already up to date (" << NEOVEX_VERSION << ").";
    return;
  }

  LOG(INFO) << "[Neovex] New version available: " << *tag;

  // Find the mini_installer.exe asset.
  const base::ListValue* assets = dict.FindList("assets");
  if (!assets) {
    return;
  }

  for (const auto& asset_val : *assets) {
    if (!asset_val.is_dict()) continue;
    const base::DictValue& asset = asset_val.GetDict();
    const std::string* name = asset.FindString("name");
    if (!name || *name != "mini_installer.exe") continue;

    const std::string* url = asset.FindString("browser_download_url");
    if (!url) continue;

    download_url_ = *url;
    break;
  }

  if (download_url_.empty()) {
    LOG(WARNING) << "[Neovex] No mini_installer.exe asset found.";
    return;
  }

  // Ensure the updates directory exists (on a background thread).
  base::FilePath updates_dir = GetUpdatesDir();
  if (updates_dir.empty()) {
    return;
  }

  base::ThreadPool::PostTaskAndReply(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(
          [](base::FilePath dir) { base::CreateDirectory(dir); },
          updates_dir),
      base::BindOnce(
          [](base::WeakPtr<UpdateChecker> self) {
            if (!self) return;
            // Now download the installer.
            auto request = std::make_unique<network::ResourceRequest>();
            request->url = GURL(self->download_url_);
            request->credentials_mode =
                network::mojom::CredentialsMode::kOmit;

            self->download_loader_ = network::SimpleURLLoader::Create(
                std::move(request), kTrafficAnnotation);

            base::FilePath dest =
                GetUpdatesDir().Append(L"mini_installer.exe");
            self->download_loader_->DownloadToFile(
                self->url_loader_factory_.get(),
                base::BindOnce(&UpdateChecker::OnInstallerDownloaded,
                               self),
                dest);
          },
          weak_factory_.GetWeakPtr()));
}

void UpdateChecker::OnInstallerDownloaded(base::FilePath path) {
  if (path.empty()) {
    LOG(WARNING) << "[Neovex] Installer download failed.";
    return;
  }

  LOG(INFO) << "[Neovex] Installer downloaded to: " << path.value();

  local_state_->SetBoolean(kUpdateDownloaded, true);
  local_state_->SetString(kUpdateInstallerPath, path.AsUTF8Unsafe());
  local_state_->CommitPendingWrite();
}

// ---------- Shutdown helper ----------

void MaybeLaunchInstallerOnShutdown(PrefService* local_state) {
  if (!local_state) return;

  if (!local_state->GetBoolean(kUpdateDownloaded)) return;

  std::string path_str = local_state->GetString(kUpdateInstallerPath);
  if (path_str.empty()) return;

#if BUILDFLAG(IS_WIN)
  base::FilePath installer_path = base::FilePath::FromUTF8Unsafe(path_str);
  if (!base::PathExists(installer_path)) {
    LOG(WARNING) << "[Neovex] Installer not found at: "
                 << installer_path.value();
    // Clear the stale flags.
    local_state->SetBoolean(kUpdateDownloaded, false);
    local_state->SetString(kUpdateInstallerPath, "");
    local_state->CommitPendingWrite();
    return;
  }

  // Clear the flags before launching so we don't try again on next shutdown.
  local_state->SetBoolean(kUpdateDownloaded, false);
  local_state->SetString(kUpdateInstallerPath, "");
  // Store a dummy version so the What's New page triggers on next launch (for testing)
  local_state->SetString(kUpdateLastVersion, "0.0.0");
  local_state->CommitPendingWrite();

  // Launch the installer as a fully detached process.
  base::CommandLine cmd(installer_path);
  cmd.AppendSwitch("do-not-launch-chrome");
  base::LaunchOptions options;
  options.start_hidden = true;
  
  // BYPASS UAC: Prevent Windows "Installer Detection" from forcing an admin prompt
  // for setup.exe by setting the compatibility layer to RunAsInvoker.
  options.environment[L"__COMPAT_LAYER"] = L"RunAsInvoker";
  
  base::LaunchProcess(cmd, options);

  LOG(INFO) << "[Neovex] Launched installer: " << installer_path.value();
#endif
}

}  // namespace neovex
