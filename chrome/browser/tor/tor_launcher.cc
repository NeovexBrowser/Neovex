// Copyright 2024 Rhenium Authors. All rights reserved.
// RHENIUM TOR

#include "chrome/browser/tor/tor_launcher.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/process/launch.h"
#include "base/process/process.h"
#include "base/strings/string_number_conversions.h"

#if BUILDFLAG(IS_WIN)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

namespace rhenium {

TorLauncher::TorLauncher() = default;

TorLauncher::~TorLauncher() {
  Stop();
}

bool TorLauncher::Start() {
  if (is_started_ && IsRunning()) {
    LOG(INFO) << "RHENIUM TOR: Tor is already running";
    return true;
  }

  // RHENIUM TOR - Locate tor.exe relative to the chrome.exe directory.
  base::FilePath exe_dir;
  base::PathService::Get(base::DIR_EXE, &exe_dir);
  #if BUILDFLAG(IS_WIN)
  base::FilePath tor_path = exe_dir.Append(L"tor\\tor.exe");
#else
  base::FilePath tor_path = exe_dir.Append("tor/tor.exe");
#endif
  if (!base::PathExists(tor_path)) {
    LOG(ERROR) << "RHENIUM TOR: tor.exe not found at " << tor_path.value();
    return false;
  }

  LOG(INFO) << "RHENIUM TOR: Found tor.exe at " << tor_path.value();

  // Create a temporary directory for Tor data
  base::FilePath tor_data_dir;
  if (!base::CreateNewTempDirectory(FILE_PATH_LITERAL("rhenium_tor_"),
                                     &tor_data_dir)) {
    LOG(ERROR) << "RHENIUM TOR: Failed to create temp directory for Tor data";
    return false;
  }

  // Build the Tor command line
  base::CommandLine tor_cmd(tor_path);
  tor_cmd.AppendArg("--SOCKSPort");
  tor_cmd.AppendArg("9050");
  tor_cmd.AppendArg("--DataDirectory");
  tor_cmd.AppendArgPath(tor_data_dir);

  // RHENIUM TOR - Tell Tor to log to our log file
  tor_log_file_ = tor_data_dir.Append(FILE_PATH_LITERAL("tor.log"));
  tor_cmd.AppendArg("--Log");
  tor_cmd.AppendArg("notice file " + tor_log_file_.AsUTF8Unsafe());

  LOG(INFO) << "RHENIUM TOR: Launching tor with command: "
            << tor_cmd.GetCommandLineString();

  // Launch the Tor process
  base::LaunchOptions options;
#if BUILDFLAG(IS_WIN)
  options.start_hidden = true;
#endif

  tor_process_ = base::LaunchProcess(tor_cmd, options);
  if (!tor_process_.IsValid()) {
    LOG(ERROR) << "RHENIUM TOR: Failed to launch tor.exe";
    return false;
  }

  is_started_ = true;
  LOG(INFO) << "RHENIUM TOR: Tor process launched successfully (PID: "
            << tor_process_.Pid() << ")";
  return true;
}

void TorLauncher::Stop() {
  if (tor_process_.IsValid()) {
    LOG(INFO) << "RHENIUM TOR: Stopping Tor process";
    tor_process_.Terminate(0, false);
    tor_process_.Close();
  }
  is_started_ = false;
  is_ready_ = false;
}

bool TorLauncher::IsRunning() const {
  if (!tor_process_.IsValid()) {
    return false;
  }
  // Check if the process has exited
  int exit_code = 0;
  if (tor_process_.WaitForExitWithTimeout(base::TimeDelta(), &exit_code)) {
    return false;  // Process has exited
  }
  return true;
}

bool TorLauncher::IsReady() const {
#if BUILDFLAG(IS_WIN)
  WSADATA wsa_data;
  if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
    return false;
  }

  SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock == INVALID_SOCKET) {
    WSACleanup();
    return false;
  }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(9050);
  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

  int result = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
  closesocket(sock);
  WSACleanup();

  return result == 0;
#else
  return false;
#endif
}

std::string TorLauncher::GetLogs() const {
  if (tor_log_file_.empty()) {
    return std::string();
  }

  std::string contents;
  if (!base::ReadFileToString(tor_log_file_, &contents)) {
    return std::string();
  }

  // Return the last ~10KB of log data to avoid huge transfers
  const size_t kMaxLogSize = 10240;
  if (contents.size() > kMaxLogSize) {
    contents = contents.substr(contents.size() - kMaxLogSize);
  }

  return contents;
}

}  // namespace rhenium
