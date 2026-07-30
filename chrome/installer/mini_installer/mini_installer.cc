// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(crbug.com/40285824): Remove this and convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

// mini_installer.exe is the first exe that is run when chrome is being
// installed or upgraded. It is designed to be extremely small (~5KB with no
// extra resources linked) and it has two main jobs:
//   1) unpack the resources (possibly decompressing some)
//   2) run the real installer (setup.exe) with appropriate flags.
//
// In order to be really small the app doesn't link against the CRT and
// defines the following compiler/linker flags:
//   EnableIntrinsicFunctions="true" compiler: /Oi
//   BasicRuntimeChecks="0"
//   BufferSecurityCheck="false" compiler: /GS-
//   EntryPointSymbol="MainEntryPoint" linker: /ENTRY
//       /ENTRY also stops the CRT from being pulled in and does this more
//       precisely than /NODEFAULTLIB
//   OptimizeForWindows98="1" linker: /OPT:NOWIN98
//   linker: /SAFESEH:NO

#include "chrome/installer/mini_installer/mini_installer.h"

#include <windows.h>


// #define needed to link in RtlGenRandom(), a.k.a. SystemFunction036.  See the
// "Community Additions" comment on MSDN here:
// http://msdn.microsoft.com/en-us/library/windows/desktop/aa387694.aspx
#define SystemFunction036 NTAPI SystemFunction036
#include <NTSecAPI.h>
#undef SystemFunction036

#include <sddl.h>
#include <shellapi.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <algorithm>
#include <initializer_list>

#include "build/branding_buildflags.h"
#include "chrome/installer/mini_installer/appid.h"
#include "chrome/installer/mini_installer/configuration.h"
#include "chrome/installer/mini_installer/decompress.h"
#include "chrome/installer/mini_installer/delete_with_retry.h"
#include "chrome/installer/mini_installer/enumerate_resources.h"
#include "chrome/installer/mini_installer/memory_range.h"
#include "chrome/installer/mini_installer/mini_file.h"
#include "chrome/installer/mini_installer/mini_installer_constants.h"
#include "chrome/installer/mini_installer/regkey.h"
#include "chrome/installer/mini_installer/write_to_disk.h"

namespace mini_installer {

// Deletes |path|, updating |max_delete_attempts| if more attempts were taken
// than indicated in |max_delete_attempts|.
void DeleteWithRetryAndMetrics(const wchar_t* path, int& max_delete_attempts) {
  int attempts = 0;
  DeleteWithRetry(path, attempts);
  if (attempts > max_delete_attempts) {
    max_delete_attempts = attempts;
  }
}

// TODO(grt): Frame this in terms of whether or not the brand supports
// integration with Omaha, where Google Update is the Google-specific fork of
// the open-source Omaha project.
#if BUILDFLAG(GOOGLE_CHROME_BRANDING)
// Opens the Google Update ClientState key for the current install mode.
bool OpenInstallStateKey(const Configuration& configuration, RegKey* key) {
  const HKEY root_key =
      configuration.is_system_level() ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
  const wchar_t* app_guid = configuration.chrome_app_guid();
  const REGSAM key_access = KEY_QUERY_VALUE | KEY_SET_VALUE;

  return OpenClientStateKey(root_key, app_guid, key_access, key) ==
         ERROR_SUCCESS;
}

// Writes install results into the registry where it is read by Google Update.
// Don't write anything if there is already a result present, likely
// written by setup.exe.
void WriteInstallResults(const Configuration& configuration,
                         ProcessExitResult result) {
  // Calls to setup.exe will write a "success" result if everything was good
  // so we don't need to write anything from here.
  if (result.IsSuccess()) {
    return;
  }

  // Write the value in Chrome ClientState key.
  RegKey key;
  DWORD value;
  if (OpenInstallStateKey(configuration, &key)) {
    if (key.ReadDWValue(kInstallerResultRegistryValue, &value) !=
            ERROR_SUCCESS ||
        value == 0) {
      key.WriteDWValue(kInstallerResultRegistryValue,
                       result.exit_code ? 1 /* FAILED_CUSTOM_ERROR */
                                        : 0 /* SUCCESS */);
      key.WriteDWValue(kInstallerErrorRegistryValue, result.exit_code);
      key.WriteDWValue(kInstallerExtraCode1RegistryValue, result.windows_error);
    }
  }
}

// Success metric reporting ----------------------------------------------------

// A single DWORD value may be written to the ExtraCode1 registry value on
// success. This is used to report a sample for a metric of a specific category.

// Categories of metrics written into ExtraCode1 on success. Values should not
// be reordered or reused unless the population reporting such categories
// becomes insiginficant or is filtered out based on release version.
enum MetricCategory : uint16_t {
  // The sample 0 indicates that %TMP% was used to hold the work dir. Active
  // from release 86.0.4237.0 through 88.0.4313.0.
  // kTemporaryDirectoryWithFallback = 1,

  // The sample 0 indicates that CWD was used to hold the work dir. Active from
  // release 86.0.4237.0 through 88.0.4313.0.
  // kTemporaryDirectoryWithoutFallback = 2,

  // Values indicate the maximum number of retries needed to delete a file or
  // directory via DeleteWithRetry. Active from release 88.0.4314.0.
  kMaxDeleteRetryCount = 3,
};

using MetricSample = uint16_t;

// Returns an ExtraCode1 value encoding a sample for a particular category.
constexpr DWORD MetricToExtraCode1(MetricCategory category,
                                   MetricSample sample) {
  return category << 16 | sample;
}

// Writes the value |extra_code_1| into ExtraCode1 for reporting by Omaha.
void WriteExtraCode1(const Configuration& configuration, DWORD extra_code_1) {
  // Write the value in Chrome ClientState key.
  RegKey key;
  if (OpenInstallStateKey(configuration, &key)) {
    key.WriteDWValue(kInstallerExtraCode1RegistryValue, extra_code_1);
  }
}

#endif  // BUILDFLAG(GOOGLE_CHROME_BRANDING)

// Calls CreateProcess with good default parameters and waits for the process to
// terminate returning the process exit code. In case of CreateProcess failure,
// returns a results object with the provided codes as follows:
// - ERROR_FILE_NOT_FOUND: (file_not_found_code, attributes of setup.exe).
// - ERROR_PATH_NOT_FOUND: (path_not_found_code, attributes of setup.exe).
// - Otherwise: (generic_failure_code, CreateProcess error code).
// In case of error waiting for the process to exit, returns a results object
// with (WAIT_FOR_PROCESS_FAILED, last error code). Otherwise, returns a results
// object with the subprocess's exit code.
ProcessExitResult RunProcessAndWait(const wchar_t* exe_path,
                                    wchar_t* cmdline,
                                    DWORD file_not_found_code,
                                    DWORD path_not_found_code,
                                    DWORD generic_failure_code) {
  STARTUPINFOW si = {sizeof(si)};
  PROCESS_INFORMATION pi = {0};
  if (!::CreateProcess(exe_path, cmdline, nullptr, nullptr, FALSE,
                       CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
    // Split specific failure modes. If setup.exe couldn't be launched because
    // its file/path couldn't be found, report its attributes in ExtraCode1.
    // This will help diagnose the prevalence of launch failures due to Image
    // File Execution Options tampering. See https://crbug.com/41290422 for more
    // details.
    const DWORD last_error = ::GetLastError();
    const DWORD attributes = ::GetFileAttributes(exe_path);
    switch (last_error) {
      case ERROR_FILE_NOT_FOUND:
        return ProcessExitResult(file_not_found_code, attributes);
      case ERROR_PATH_NOT_FOUND:
        return ProcessExitResult(path_not_found_code, attributes);
      default:
        break;
    }
    // Lump all other errors into a distinct failure bucket.
    return ProcessExitResult(generic_failure_code, last_error);
  }

  ::CloseHandle(pi.hThread);

  DWORD exit_code = SUCCESS_EXIT_CODE;
  while (true) {
    DWORD wr = ::MsgWaitForMultipleObjects(1, &pi.hProcess, FALSE, INFINITE, QS_ALLINPUT);
    if (wr == WAIT_OBJECT_0) {
      break;  // Process finished.
    } else if (wr == WAIT_OBJECT_0 + 1) {
      // Messages are available, pump them to keep the UI responsive.
      MSG msg;
      while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
          // If we receive a quit message, post it back and return an error to abort.
          ::PostQuitMessage(static_cast<int>(msg.wParam));
          ::CloseHandle(pi.hProcess);
          return ProcessExitResult(WAIT_FOR_PROCESS_FAILED, ERROR_PROCESS_ABORTED);
        }
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
      }
    } else {
      // Wait failed.
      return ProcessExitResult(WAIT_FOR_PROCESS_FAILED, ::GetLastError());
    }
  }

  if (!::GetExitCodeProcess(pi.hProcess, &exit_code)) {
    return ProcessExitResult(WAIT_FOR_PROCESS_FAILED, ::GetLastError());
  }

  ::CloseHandle(pi.hProcess);

  return ProcessExitResult(exit_code);
}

void AppendCommandLineFlags(const wchar_t* command_line,
                            CommandString* buffer) {
  // The program name (the first argument parsed by CommandLineToArgvW) is
  // delimited by whitespace or a double quote based on the first character of
  // the full command line string. Use the same logic here to scan past the
  // program name in the program's command line (obtained during startup from
  // GetCommandLine). See
  // http://www.windowsinspired.com/how-a-windows-programs-splits-its-command-line-into-individual-arguments/
  // for gory details regarding how CommandLineToArgvW works.
  wchar_t a_char = 0;
  if (*command_line == L'"') {
    // Scan forward past the closing double quote.
    ++command_line;
    while (true) {
      a_char = *command_line;
      if (!a_char) {
        break;
      }
      ++command_line;
      if (a_char == L'"') {
        a_char = *command_line;
        break;
      }
    }  // postcondition: |a_char| contains the character at *command_line.
  } else {
    // Scan forward for the first space or tab character.
    while (true) {
      a_char = *command_line;
      if (!a_char || a_char == L' ' || a_char == L'\t') {
        break;
      }
      ++command_line;
    }  // postcondition: |a_char| contains the character at *command_line.
  }

  if (!a_char) {
    return;
  }

  // Append a space if |command_line| doesn't begin with one.
  if (a_char != ' ' && a_char != '\t' && !buffer->append(L" ")) {
    return;
  }
  buffer->append(command_line);
}

namespace {

// A ResourceEnumeratorDelegate that captures the resource name and data range
// for the chrome 7zip archive and the setup.
class ChromeResourceDelegate : public ResourceEnumeratorDelegate {
 public:
  ChromeResourceDelegate(PathString& archive_name,
                         MemoryRange& archive_range,
                         PathString& setup_name,
                         MemoryRange& setup_range,
                         DWORD& error_code)
      : archive_name_(archive_name),
        archive_range_(archive_range),
        setup_name_(setup_name),
        setup_range_(setup_range),
        error_code_(error_code) {}
  bool OnResource(const wchar_t* name, const MemoryRange& data_range) override;

 private:
  PathString& archive_name_;
  MemoryRange& archive_range_;
  PathString& setup_name_;
  MemoryRange& setup_range_;
  DWORD& error_code_;
};

// Returns false to stop enumeration on unexpected resource names, duplicate
// archive resources, or string overflow.
bool ChromeResourceDelegate::OnResource(const wchar_t* name,
                                        const MemoryRange& data_range) {
  if (StrStartsWith(name, kChromeArchivePrefix)) {
    if (!archive_range_.empty()) {
      error_code_ = ERROR_TOO_MANY_NAMES;
      return false;  // Break: duplicate resource name.
    }
    if (!archive_name_.assign(name)) {
      error_code_ = ERROR_FILENAME_EXCED_RANGE;
      return false;  // Break: resource name is too long.
    }
    archive_range_ = data_range;
  } else if (StrStartsWith(name, kSetupPrefix)) {
    if (!setup_range_.empty()) {
      error_code_ = ERROR_TOO_MANY_NAMES;
      return false;  // Break: duplicate resource name.
    }
    if (!setup_name_.assign(name)) {
      error_code_ = ERROR_FILENAME_EXCED_RANGE;
      return false;  // Break: resource name is too long.
    }
    setup_range_ = data_range;
  } else {
    error_code_ = ERROR_INVALID_DATA;
    return false;  // Break: unexpected resource name.
  }
  return true;  // Continue: advance to the next resource.
}

#if defined(COMPONENT_BUILD)
// A ResourceEnumeratorDelegate that writes all resources to disk in a given
// directory (which must end with a path separator).
class ResourceWriterDelegate : public ResourceEnumeratorDelegate {
 public:
  explicit ResourceWriterDelegate(const wchar_t* base_path)
      : base_path_(base_path) {}
  bool OnResource(const wchar_t* name, const MemoryRange& data_range) override;

 private:
  const wchar_t* const base_path_;
};

bool ResourceWriterDelegate::OnResource(const wchar_t* name,
                                        const MemoryRange& data_range) {
  PathString full_path;
  return (!data_range.empty() && full_path.assign(base_path_) &&
          full_path.append(name) && WriteToDisk(data_range, full_path.get()));
}

// A ResourceEnumeratorDelegate that deletes the file corresponding to each
// resource from a given directory (which must end with a path separator).
class ResourceDeleterDelegate : public ResourceEnumeratorDelegate {
 public:
  explicit ResourceDeleterDelegate(const wchar_t* base_path)
      : base_path_(base_path) {}
  bool OnResource(const wchar_t* name, const MemoryRange& data_range) override;

 private:
  const wchar_t* const base_path_;
};

bool ResourceDeleterDelegate::OnResource(const wchar_t* name,
                                         const MemoryRange& data_range) {
  PathString full_path;
  if (full_path.assign(base_path_) && full_path.append(name)) {
    // Do not record metrics for these deletes, as they are not done for release
    // builds.
    int attempts;
    DeleteWithRetry(full_path.get(), attempts);
  }

  return true;  // Continue enumeration.
}
#endif  // defined(COMPONENT_BUILD)

}  // namespace

ProcessExitResult UnpackBinaryResources(HMODULE module,
                                        const wchar_t* base_path,
                                        PathString& setup_path,
                                        PathString& archive_path,
                                        ResourceTypeString& archive_type,
                                        int& max_delete_attempts) {
  // Generate the setup.exe path where we uncompress setup resource.
  ResourceTypeString setup_type;
  PathString setup_name;
  MemoryRange setup_range;
  PathString archive_name;
  MemoryRange archive_range;

  // Scan through all types of resources looking for the chrome archive (which
  // is expected to be either a B7 chrome.packed.7z or a BN chrome.7z) and
  // installer (which is expected to be a BL setup.ex_, or a BN setup.exe).
  for (const auto* type :
       {kLZMAResourceType, kLZCResourceType, kBinResourceType}) {
    DWORD error_code = ERROR_SUCCESS;
    // We ignore the result of EnumerateResources here because a non-success
    // does not always indicate an error occurred.
    EnumerateResources(
        ChromeResourceDelegate(archive_name, archive_range, setup_name,
                               setup_range, error_code),
        module, type);
    // `error_code` will have been modified by the delegate in case of error.
    if (error_code != ERROR_SUCCESS) {
      return ProcessExitResult(archive_type.empty()
                                   ? UNABLE_TO_EXTRACT_CHROME_ARCHIVE
                                   : UNABLE_TO_EXTRACT_SETUP_EXE,
                               error_code);
    }
    // If this iteration found either resource, remember its type.
    if (archive_type.empty() && !archive_range.empty()) {
      if (!archive_type.assign(type)) {
        return ProcessExitResult(UNABLE_TO_EXTRACT_SETUP,
                                 UNABLE_TO_EXTRACT_CHROME_ARCHIVE);
      }
    }
    if (setup_type.empty() && !setup_range.empty()) {
      if (!setup_type.assign(type)) {
        return ProcessExitResult(UNABLE_TO_EXTRACT_SETUP, ERROR_INCORRECT_SIZE);
      }
    }
    // Keep searching even if both were found so that an ChromeResourceDelegate
    // will propagate an error from `EnumerateResources` in case of duplicate
    // resources.
  }
  if (archive_range.empty()) {
    return ProcessExitResult(UNABLE_TO_EXTRACT_CHROME_ARCHIVE,
                             ERROR_FILE_NOT_FOUND);
  }
  if (setup_range.empty()) {
    return ProcessExitResult(UNABLE_TO_EXTRACT_SETUP_EXE, ERROR_FILE_NOT_FOUND);
  }

  // Write the archive to disk.
  if (!archive_path.assign(base_path) ||
      !archive_path.append(archive_name.get())) {
    return ProcessExitResult(PATH_STRING_OVERFLOW);
  }
  if (!WriteToDisk(archive_range, archive_path.get())) {
    return ProcessExitResult(UNABLE_TO_EXTRACT_CHROME_ARCHIVE,
                             ::GetLastError());
  }

  // Extract directly to "setup.exe" if the resource is not compressed.
  if (!setup_path.assign(base_path) ||
      !setup_path.append(setup_type.compare(kBinResourceType) == 0
                             ? kSetupExe
                             : setup_name.get())) {
    return ProcessExitResult(PATH_STRING_OVERFLOW);
  }

  // Write the setup binary, possibly compressed, to disk.
  if (!WriteToDisk(setup_range, setup_path.get())) {
    return ProcessExitResult(UNABLE_TO_EXTRACT_SETUP, ::GetLastError());
  }

  ProcessExitResult exit_code = ProcessExitResult(SUCCESS_EXIT_CODE);

  if (setup_type.compare(kLZCResourceType) == 0) {
    PathString setup_dest_path;
    if (!setup_dest_path.assign(base_path) ||
        !setup_dest_path.append(kSetupExe)) {
      return ProcessExitResult(PATH_STRING_OVERFLOW);
    }
    bool success =
        mini_installer::Expand(setup_path.get(), setup_dest_path.get());
    DeleteWithRetryAndMetrics(setup_path.get(), max_delete_attempts);

    if (!success) {
      exit_code = ProcessExitResult(UNABLE_TO_EXTRACT_SETUP_EXE);
    }
    setup_path.assign(setup_dest_path);
  }

#if defined(COMPONENT_BUILD)
  if (exit_code.IsSuccess()) {
    // Extract the modules in component build required by setup.exe.
    if (!EnumerateResources(ResourceWriterDelegate(base_path), module,
                            kDepResourceType)) {
      return ProcessExitResult(UNABLE_TO_EXTRACT_SETUP, ::GetLastError());
    }
  }
#endif  // defined(COMPONENT_BUILD)

  return exit_code;
}

// Executes setup.exe, waits for it to finish and returns the exit code.
ProcessExitResult RunSetup(const Configuration& configuration,
                           const wchar_t* archive_path,
                           const wchar_t* setup_path,
                           bool compressed_archive) {
  // Get the path to setup.exe.
  PathString setup_exe;
  if (!setup_exe.assign(setup_path)) {
    return ProcessExitResult(COMMAND_STRING_OVERFLOW);
  }

  // There could be three full paths in the command line for setup.exe (path
  // to exe itself, path to archive and path to log file), so we declare
  // total size as three + one additional to hold command line options.
  CommandString cmd_line;
  // Put the quoted path to setup.exe in cmd_line first.
  if (!cmd_line.assign(L"\"") || !cmd_line.append(setup_exe.get()) ||
      !cmd_line.append(L"\"")) {
    return ProcessExitResult(COMMAND_STRING_OVERFLOW);
  }

  // Append the command line param for chrome archive file.
  const wchar_t* const archive_switch =
      compressed_archive ? kCmdInstallArchive : kCmdUncompressedArchive;
  if (!cmd_line.append(L" --") || !cmd_line.append(archive_switch) ||
      !cmd_line.append(L"=\"") || !cmd_line.append(archive_path) ||
      !cmd_line.append(L"\"")) {
    return ProcessExitResult(COMMAND_STRING_OVERFLOW);
  }

  // Get any command line option specified for mini_installer and pass them
  // on to setup.exe
  AppendCommandLineFlags(configuration.command_line(), &cmd_line);

  if (configuration.is_system_level()) {
    const wchar_t* search = cmd_line.get();
    bool has_system_level = false;
    size_t search_len = SafeStrLen(search, cmd_line.capacity());
    for (size_t i = 0; i + 14 <= search_len; ++i) {
      if (search[i] == L'-' && search[i + 1] == L'-') {
        bool match = true;
        const wchar_t* target = L"system-level";
        for (size_t j = 0; j < 12; ++j) {
          wchar_t a = search[i + 2 + j];
          wchar_t b = target[j];
          if (a >= L'A' && a <= L'Z') {
            a += (L'a' - L'A');
          }
          if (a != b) {
            match = false;
            break;
          }
        }
        if (match) {
          has_system_level = true;
          break;
        }
      }
    }
    if (!has_system_level) {
      if (!cmd_line.append(L" --system-level")) {
        return ProcessExitResult(COMMAND_STRING_OVERFLOW);
      }
    }
  }

  return RunProcessAndWait(setup_exe.get(), cmd_line.get(),
                           RUN_SETUP_FAILED_FILE_NOT_FOUND,
                           RUN_SETUP_FAILED_PATH_NOT_FOUND,
                           RUN_SETUP_FAILED_COULD_NOT_CREATE_PROCESS);
}

// Deletes the files extracted by UnpackBinaryResources and the work directory
// created by GetWorkDir.
void DeleteExtractedFiles(HMODULE module,
                          const PathString& archive_path,
                          const PathString& setup_path,
                          const PathString& base_path,
                          int& max_delete_attempts) {
  if (!archive_path.empty()) {
    DeleteWithRetryAndMetrics(archive_path.get(), max_delete_attempts);
  }
  if (!setup_path.empty()) {
    DeleteWithRetryAndMetrics(setup_path.get(), max_delete_attempts);
  }

#if defined(COMPONENT_BUILD)
  // Delete the modules in a component build extracted for use by setup.exe.
  EnumerateResources(ResourceDeleterDelegate(base_path.get()), module,
                     kDepResourceType);
#endif  // defined(COMPONENT_BUILD)

  // Delete the temp dir (if it is empty, otherwise fail).
  DeleteWithRetryAndMetrics(base_path.get(), max_delete_attempts);
}

// Returns true if the supplied path supports ACLs.
bool IsAclSupportedForPath(const wchar_t* path) {
  PathString volume;
  DWORD flags = 0;
  return ::GetVolumePathName(path, volume.get(),
                             static_cast<DWORD>(volume.capacity())) &&
         ::GetVolumeInformation(volume.get(), nullptr, 0, nullptr, nullptr,
                                &flags, nullptr, 0) &&
         (flags & FILE_PERSISTENT_ACLS);
}

// Retrieves the SID of the default owner for objects created by this user
// token (accounting for different behavior under UAC elevation, etc.).
// NOTE: On success the |sid| parameter must be freed with LocalFree().
bool GetCurrentOwnerSid(wchar_t** sid) {
  HANDLE token;
  if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &token)) {
    return false;
  }

  DWORD size = 0;
  bool result = false;
  // We get the TokenOwner rather than the TokenUser because e.g. under UAC
  // elevation we want the admin to own the directory rather than the user.
  ::GetTokenInformation(token, TokenOwner, nullptr, 0, &size);
  if (size && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
    if (TOKEN_OWNER* owner =
            reinterpret_cast<TOKEN_OWNER*>(::LocalAlloc(LPTR, size))) {
      if (::GetTokenInformation(token, TokenOwner, owner, size, &size)) {
        result = !!::ConvertSidToStringSid(owner->Owner, sid);
      }
      ::LocalFree(owner);
    }
  }
  ::CloseHandle(token);
  return result;
}

// Populates |sd| suitable for use when creating directories within |path| with
// ACLs allowing access to only the current owner, admin, and system.
// NOTE: On success the |sd| parameter must be freed with LocalFree().
bool SetSecurityDescriptor(const wchar_t* path, PSECURITY_DESCRIPTOR* sd) {
  *sd = nullptr;
  // We succeed without doing anything if ACLs aren't supported.
  if (!IsAclSupportedForPath(path)) {
    return true;
  }

  wchar_t* sid = nullptr;
  if (!GetCurrentOwnerSid(&sid)) {
    return false;
  }

  // The largest SID is under 200 characters, so 300 should give enough slack.
  StackString<300> sddl;
  bool result = sddl.append(
                    L"D:PAI"         // Protected, auto-inherited DACL.
                    L"(A;;FA;;;BA)"  // Admin: Full control.
                    L"(A;OIIOCI;GA;;;BA)"
                    L"(A;;FA;;;SY)"  // System: Full control.
                    L"(A;OIIOCI;GA;;;SY)"
                    L"(A;OIIOCI;GA;;;CO)"  // Owner: Full control.
                    L"(A;;FA;;;") &&
                sddl.append(sid) && sddl.append(L")");
  if (result) {
    result = !!::ConvertStringSecurityDescriptorToSecurityDescriptor(
        sddl.get(), SDDL_REVISION_1, sd, nullptr);
  }

  ::LocalFree(sid);
  return result;
}

bool GetModuleDir(HMODULE module, PathString* directory) {
  DWORD len = ::GetModuleFileName(module, directory->get(),
                                  static_cast<DWORD>(directory->capacity()));
  if (!len || len >= directory->capacity()) {
    return false;  // Failed to get module path.
  }

  // Chop off the basename of the path.
  wchar_t* name = GetNameFromPathExt(directory->get(), len);
  if (name == directory->get()) {
    return false;  // No path separator found.
  }

  *name = L'\0';

  return true;
}

// Creates a temporary directory under |base_path| and returns the full path
// of created directory in |work_dir|. If successful return true, otherwise
// false.  When successful, the returned |work_dir| will always have a trailing
// backslash and this function requires that |base_path| always includes a
// trailing backslash as well.
// We do not use GetTempFileName here to avoid running into AV software that
// might hold on to the temp file as soon as we create it and then we can't
// delete it and create a directory in its place.  So, we use our own mechanism
// for creating a directory with a hopefully-unique name.  In the case of a
// collision, we retry a few times with a new name before failing.
bool CreateWorkDir(const wchar_t* base_path,
                   PathString* work_dir,
                   ProcessExitResult* exit_code) {
  *exit_code = ProcessExitResult(PATH_STRING_OVERFLOW);
  if (!work_dir->assign(base_path) || !work_dir->append(kTempPrefix)) {
    return false;
  }

  // Store the location where we'll append the id.
  size_t end = work_dir->length();

  // Check if we'll have enough buffer space to continue.
  // The name of the directory will use up 11 chars and then we need to append
  // the trailing backslash and a terminator.  We've already added the prefix
  // to the buffer, so let's just make sure we've got enough space for the rest.
  if ((work_dir->capacity() - end) < (_countof("fffff.tmp") + 1)) {
    return false;
  }

  // Add an ACL if supported by the filesystem. Otherwise system-level installs
  // are potentially vulnerable to file squatting attacks.
  SECURITY_ATTRIBUTES sa = {};
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  if (!SetSecurityDescriptor(base_path, &sa.lpSecurityDescriptor)) {
    *exit_code =
        ProcessExitResult(UNABLE_TO_SET_DIRECTORY_ACL, ::GetLastError());
    return false;
  }

  unsigned int id;
  *exit_code = ProcessExitResult(UNABLE_TO_GET_WORK_DIRECTORY);
  for (int max_attempts = 10; max_attempts; --max_attempts) {
    ::RtlGenRandom(&id, sizeof(id));  // Try a different name.

    // This converts 'id' to a string in the format "78563412" on windows
    // because of little endianness, but we don't care since it's just
    // a name. Since we checked capaity at the front end, we don't need to
    // duplicate it here.
    HexEncode(&id, sizeof(id), work_dir->get() + end,
              work_dir->capacity() - end);

    // We only want the first 5 digits to remain within the 8.3 file name
    // format (compliant with previous implementation).
    work_dir->truncate_at(end + 5);

    // for consistency with the previous implementation which relied on
    // GetTempFileName, we append the .tmp extension.
    work_dir->append(L".tmp");

    if (::CreateDirectory(work_dir->get(),
                          sa.lpSecurityDescriptor ? &sa : nullptr)) {
      // Yay!  Now let's just append the backslash and we're done.
      work_dir->append(L"\\");
      *exit_code = ProcessExitResult(SUCCESS_EXIT_CODE);
      break;
    }
  }

  if (sa.lpSecurityDescriptor) {
    LocalFree(sa.lpSecurityDescriptor);
  }

  return exit_code->IsSuccess();
}

// Creates and returns a temporary directory in |work_dir| that can be used to
// extract mini_installer payload. |work_dir| ends with a path separator.
// Returns true if |work_dir| is available for use, or false in case of error
// (indicated by |exit_code|).
bool GetWorkDir(HMODULE module,
                PathString* work_dir,
                ProcessExitResult* exit_code) {
  PathString base_path;

  // Create a directory next to the current module.
  return GetModuleDir(module, &base_path) &&
         CreateWorkDir(base_path.get(), work_dir, exit_code);
}

// ---- Neovex Progress Window -------------------------------------------------
// Modern borderless installer UI.  Owner-drawn with GDI — no themed controls.
// Non-interactive: the user cannot close, move, or resize the window.

namespace {

// Layout constants.
static const int kWindowW = 440;
static const int kWindowH = 156;
static const int kPadX = 28;           // horizontal padding
static const int kTitleY = 24;         // title baseline offset
static const int kStatusY = 48;        // status text baseline offset
static const int kBarY = 78;           // progress bar top
static const int kBarH = 6;            // progress bar height
static const int kBarRadius = 3;       // progress bar corner radius

static const int kBtnW = 90;
static const int kBtnH = 32;
static const int kBtnRadius = 4;
static const int kBtnY = 100;

// Colors — dark matte palette.
static const COLORREF kBgColor = RGB(14, 15, 19);       // #0E0F13
static const COLORREF kBorderColor = RGB(32, 34, 42);   // #20222A
static const COLORREF kTitleColor = RGB(237, 238, 242);  // #EDEEF2
static const COLORREF kStatusColor = RGB(130, 134, 148); // #828694
static const COLORREF kBarTrack = RGB(26, 27, 35);       // #1A1B23
static const COLORREF kBarFill = RGB(99, 102, 241);      // #6366F1 — indigo

static const COLORREF kBtnBg = RGB(99, 102, 241);        // #6366F1
static const COLORREF kBtnHover = RGB(79, 70, 229);      // #4F46E5
static const COLORREF kBtnPress = RGB(67, 56, 202);      // #4338CA
static const COLORREF kBtnText = RGB(255, 255, 255);

// Per-window state kept in GWLP_USERDATA.
struct ProgressState {
  int progress;          // 0-100, or -1 for marquee
  int marquee_offset;    // animated offset for marquee mode
  bool is_complete;
  bool button_hovered;
  bool button_pressed;
  wchar_t title[64];
  wchar_t status[128];
  HFONT title_font;
  HFONT status_font;
  UINT_PTR timer_id;
};

static const wchar_t kProgressWindowClass[] = L"NeovexInstallerProgress";

// Fills a rounded rectangle into the given DC.
void FillRoundRect(HDC hdc, int x, int y, int w, int h, int r, COLORREF color) {
  HBRUSH brush = ::CreateSolidBrush(color);
  HPEN pen = ::CreatePen(PS_SOLID, 0, color);
  HGDIOBJ old_brush = ::SelectObject(hdc, brush);
  HGDIOBJ old_pen = ::SelectObject(hdc, pen);
  ::RoundRect(hdc, x, y, x + w, y + h, r * 2, r * 2);
  ::SelectObject(hdc, old_pen);
  ::SelectObject(hdc, old_brush);
  ::DeleteObject(pen);
  ::DeleteObject(brush);
}

// Paints the entire client area.
void PaintWindow(HWND hwnd, HDC hdc) {
  ProgressState* ps = reinterpret_cast<ProgressState*>(
      ::GetWindowLongPtr(hwnd, GWLP_USERDATA));
  if (!ps) {
    return;
  }

  RECT rc;
  ::GetClientRect(hwnd, &rc);
  int cw = rc.right;

  // Background fill.
  HBRUSH bg = ::CreateSolidBrush(kBgColor);
  ::FillRect(hdc, &rc, bg);
  ::DeleteObject(bg);

  // 1px border inset.
  HBRUSH border_brush = ::CreateSolidBrush(kBorderColor);
  ::FrameRect(hdc, &rc, border_brush);
  ::DeleteObject(border_brush);

  // Title text.
  ::SetBkMode(hdc, TRANSPARENT);
  if (ps->title_font) {
    ::SelectObject(hdc, ps->title_font);
  }
  ::SetTextColor(hdc, kTitleColor);
  RECT title_rc = {kPadX, kTitleY, cw - kPadX, kTitleY + 22};
  ::DrawTextW(hdc, ps->title, -1, &title_rc, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);

  // Status text.
  if (ps->status_font) {
    ::SelectObject(hdc, ps->status_font);
  }
  ::SetTextColor(hdc, kStatusColor);
  RECT status_rc = {kPadX, kStatusY, cw - kPadX, kStatusY + 18};
  ::DrawTextW(hdc, ps->status, -1, &status_rc, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);

  // Progress bar track.
  int bar_w = cw - kPadX * 2;
  FillRoundRect(hdc, kPadX, kBarY, bar_w, kBarH, kBarRadius, kBarTrack);

  // Progress bar fill.
  if (ps->progress >= 0) {
    // Determinate: fill proportionally.
    int fill_w = (bar_w * ps->progress) / 100;
    if (fill_w > 0) {
      // Clamp the minimum visible width to the diameter so the rounding looks right.
      if (fill_w < kBarRadius * 2) {
        fill_w = kBarRadius * 2;
      }
      FillRoundRect(hdc, kPadX, kBarY, fill_w, kBarH, kBarRadius, kBarFill);
    }
  } else {
    // Marquee: sliding segment.
    int seg_w = bar_w / 3;
    int travel = bar_w + seg_w;
    int offset = ps->marquee_offset % travel;
    int seg_x = kPadX + offset - seg_w;

    // Clip to the track area so the segment doesn't bleed outside.
    HRGN clip = ::CreateRoundRectRgn(kPadX, kBarY, kPadX + bar_w + 1,
                                     kBarY + kBarH + 1,
                                     kBarRadius * 2, kBarRadius * 2);
    ::SelectClipRgn(hdc, clip);
    FillRoundRect(hdc, seg_x, kBarY, seg_w, kBarH, kBarRadius, kBarFill);
    ::SelectClipRgn(hdc, nullptr);
    ::DeleteObject(clip);
  }

  if (ps->is_complete) {
    int btn_x = cw - kPadX - kBtnW;
    COLORREF btn_color = ps->button_pressed ? kBtnPress : (ps->button_hovered ? kBtnHover : kBtnBg);
    FillRoundRect(hdc, btn_x, kBtnY, kBtnW, kBtnH, kBtnRadius, btn_color);
    
    ::SetBkMode(hdc, TRANSPARENT);
    if (ps->status_font) {
      ::SelectObject(hdc, ps->status_font);
    }
    ::SetTextColor(hdc, kBtnText);
    RECT btn_text_rc = { btn_x, kBtnY, btn_x + kBtnW, kBtnY + kBtnH };
    ::DrawTextW(hdc, L"Finish", -1, &btn_text_rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
  }
}

LRESULT CALLBACK ProgressWndProc(HWND hwnd,
                                 UINT msg,
                                 WPARAM wparam,
                                 LPARAM lparam) {
  switch (msg) {
    case WM_PAINT: {
      PAINTSTRUCT ps_paint;
      HDC hdc = ::BeginPaint(hwnd, &ps_paint);
      PaintWindow(hwnd, hdc);
      ::EndPaint(hwnd, &ps_paint);
      return 0;
    }
    case WM_ERASEBKGND:
      return 1;  // We handle all painting in WM_PAINT.
    case WM_MOUSEMOVE: {
      ProgressState* ps = reinterpret_cast<ProgressState*>(
          ::GetWindowLongPtr(hwnd, GWLP_USERDATA));
      if (ps && ps->is_complete) {
        POINT pt = { (short)LOWORD(lparam), (short)HIWORD(lparam) };
        RECT rc;
        ::GetClientRect(hwnd, &rc);
        int cw = rc.right;
        RECT btn_rc = {cw - kPadX - kBtnW, kBtnY, cw - kPadX, kBtnY + kBtnH};
        bool hovered = (pt.x >= btn_rc.left && pt.x <= btn_rc.right &&
                        pt.y >= btn_rc.top && pt.y <= btn_rc.bottom);
        if (hovered != ps->button_hovered) {
          ps->button_hovered = hovered;
          ::InvalidateRect(hwnd, &btn_rc, FALSE);
          
          if (hovered) {
            TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, hwnd, 0 };
            ::TrackMouseEvent(&tme);
          }
        }
      }
      return 0;
    }
    case WM_MOUSELEAVE: {
      ProgressState* ps = reinterpret_cast<ProgressState*>(
          ::GetWindowLongPtr(hwnd, GWLP_USERDATA));
      if (ps && ps->button_hovered) {
        ps->button_hovered = false;
        ps->button_pressed = false;
        RECT rc;
        ::GetClientRect(hwnd, &rc);
        int cw = rc.right;
        RECT btn_rc = {cw - kPadX - kBtnW, kBtnY, cw - kPadX, kBtnY + kBtnH};
        ::InvalidateRect(hwnd, &btn_rc, FALSE);
      }
      return 0;
    }
    case WM_LBUTTONDOWN: {
      ProgressState* ps = reinterpret_cast<ProgressState*>(
          ::GetWindowLongPtr(hwnd, GWLP_USERDATA));
      if (ps && ps->is_complete && ps->button_hovered) {
        ps->button_pressed = true;
        RECT rc;
        ::GetClientRect(hwnd, &rc);
        int cw = rc.right;
        RECT btn_rc = {cw - kPadX - kBtnW, kBtnY, cw - kPadX, kBtnY + kBtnH};
        ::InvalidateRect(hwnd, &btn_rc, FALSE);
      }
      return 0;
    }
    case WM_LBUTTONUP: {
      ProgressState* ps = reinterpret_cast<ProgressState*>(
          ::GetWindowLongPtr(hwnd, GWLP_USERDATA));
      if (ps && ps->is_complete && ps->button_pressed) {
        ps->button_pressed = false;
        if (ps->button_hovered) {
          ::DestroyWindow(hwnd);
        } else {
          RECT rc;
          ::GetClientRect(hwnd, &rc);
          int cw = rc.right;
          RECT btn_rc = {cw - kPadX - kBtnW, kBtnY, cw - kPadX, kBtnY + kBtnH};
          ::InvalidateRect(hwnd, &btn_rc, FALSE);
        }
      }
      return 0;
    }
    case WM_TIMER: {
      ProgressState* ps = reinterpret_cast<ProgressState*>(
          ::GetWindowLongPtr(hwnd, GWLP_USERDATA));
      if (ps && ps->progress < 0) {
        ps->marquee_offset += 4;
        ::InvalidateRect(hwnd, nullptr, FALSE);
      }
      return 0;
    }
    case WM_NCHITTEST:
      return HTCLIENT;  // Prevent dragging — treat everything as client area.
    case WM_DESTROY: {
      ProgressState* ps = reinterpret_cast<ProgressState*>(
          ::GetWindowLongPtr(hwnd, GWLP_USERDATA));
      if (ps) {
        if (ps->timer_id) {
          ::KillTimer(hwnd, ps->timer_id);
        }
        if (ps->title_font) {
          ::DeleteObject(ps->title_font);
        }
        if (ps->status_font) {
          ::DeleteObject(ps->status_font);
        }
        // ps itself lives on the stack in WMain — do not free it.
      }
      ::PostQuitMessage(0);
      return 0;
    }
    default:
      return ::DefWindowProc(hwnd, msg, wparam, lparam);
  }
}

// Copies src into dst, up to (max_len - 1) characters, then null-terminates.
void SafeCopy(wchar_t* dst, const wchar_t* src, int max_len) {
  int i = 0;
  while (i < max_len - 1 && src[i]) {
    dst[i] = src[i];
    ++i;
  }
  dst[i] = L'\0';
}

// Creates the progress window. Returns nullptr on failure.
HWND CreateProgressWindow(HMODULE module, ProgressState* ps) {
  // Zero-init the state.
  for (int i = 0; i < static_cast<int>(sizeof(*ps)); ++i) {
    reinterpret_cast<char*>(ps)[i] = 0;
  }
  SafeCopy(ps->title, L"Installing Neovex", 64);
  SafeCopy(ps->status, L"Preparing\x2026", 128);  // ellipsis character
  ps->progress = 0;

  // Fonts.
  ps->title_font =
      ::CreateFontW(-15, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS,
                    L"Segoe UI Variable");
  if (!ps->title_font) {
    // Fallback if Variable is not available.
    ps->title_font =
        ::CreateFontW(-15, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE,
                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                      CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS,
                      L"Segoe UI");
  }
  ps->status_font =
      ::CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS,
                    L"Segoe UI");

  WNDCLASSEXW wc = {};
  wc.cbSize = sizeof(wc);
  wc.style = CS_HREDRAW | CS_VREDRAW | CS_DROPSHADOW;
  wc.lpfnWndProc = ProgressWndProc;
  wc.hInstance = module;
  wc.hCursor = ::LoadCursor(nullptr, IDC_APPSTARTING);
  wc.hbrBackground = nullptr;  // We paint everything ourselves.
  wc.lpszClassName = kProgressWindowClass;
  ::RegisterClassExW(&wc);

  // Center on screen.
  int screen_w = ::GetSystemMetrics(SM_CXSCREEN);
  int screen_h = ::GetSystemMetrics(SM_CYSCREEN);
  int x = (screen_w - kWindowW) / 2;
  int y = (screen_h - kWindowH) / 2;

  // Borderless popup — no title bar, no system menu, no resize grip.
  HWND hwnd = ::CreateWindowExW(
      WS_EX_APPWINDOW | WS_EX_TOPMOST,
      kProgressWindowClass, L"Neovex",
      WS_POPUP | WS_VISIBLE,
      x, y, kWindowW, kWindowH,
      nullptr, nullptr, module, nullptr);
  if (!hwnd) {
    return nullptr;
  }

  // Try to set rounded corners on Windows 11+ via DwmSetWindowAttribute.
  // DWMWA_WINDOW_CORNER_PREFERENCE = 33, DWMWCP_ROUNDSMALL = 3 (8px radius).
  typedef HRESULT(WINAPI* DwmSetAttrFn)(HWND, DWORD, LPCVOID, DWORD);
  HMODULE dwm = ::LoadLibraryW(L"dwmapi.dll");
  if (dwm) {
    DwmSetAttrFn fn = reinterpret_cast<DwmSetAttrFn>(
        ::GetProcAddress(dwm, "DwmSetWindowAttribute"));
    if (fn) {
      int pref = 3;  // DWMWCP_ROUNDSMALL — subtle 8px rounding.
      fn(hwnd, 33, &pref, sizeof(pref));
    }
    // Don't FreeLibrary — DWM stays loaded for the process lifetime anyway.
  }

  // Store state pointer.
  ::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ps));

  // Start a timer for marquee animation (will only animate when progress < 0).
  ps->timer_id = ::SetTimer(hwnd, 1, 16, nullptr);  // ~60fps

  ::ShowWindow(hwnd, SW_SHOW);
  ::UpdateWindow(hwnd);
  return hwnd;
}

// Pumps pending messages so the window stays responsive.
void PumpMessages() {
  MSG msg;
  while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
    if (msg.message == WM_QUIT) {
      ::PostQuitMessage(static_cast<int>(msg.wParam));
      break;
    }
    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
  }
}

// Sets the progress bar value (0-100) and status text.
void SetProgress(HWND hwnd, int value, const wchar_t* status) {
  if (!hwnd) {
    return;
  }
  ProgressState* ps = reinterpret_cast<ProgressState*>(
      ::GetWindowLongPtr(hwnd, GWLP_USERDATA));
  if (!ps) {
    return;
  }
  ps->progress = value;
  if (status) {
    SafeCopy(ps->status, status, 128);
  }
  ::InvalidateRect(hwnd, nullptr, FALSE);
  PumpMessages();
}

// Switches the progress bar to indeterminate marquee mode.
void SetMarquee(HWND hwnd, const wchar_t* status) {
  if (!hwnd) {
    return;
  }
  ProgressState* ps = reinterpret_cast<ProgressState*>(
      ::GetWindowLongPtr(hwnd, GWLP_USERDATA));
  if (!ps) {
    return;
  }
  ps->progress = -1;
  ps->marquee_offset = 0;
  if (status) {
    SafeCopy(ps->status, status, 128);
  }
  ::InvalidateRect(hwnd, nullptr, FALSE);
  PumpMessages();
}

}  // namespace
// ---- End Neovex Progress Window ---------------------------------------------

ProcessExitResult WMain(HMODULE module) {
  ProcessExitResult exit_code = ProcessExitResult(SUCCESS_EXIT_CODE);

  // Parse configuration from the command line and resources.
  Configuration configuration;
  if (!configuration.Initialize()) {
    return ProcessExitResult(GENERIC_INITIALIZATION_FAILURE, ::GetLastError());
  }

  // Exit early if an invalid switch (e.g., "--chrome-frame") was found on the
  // command line.
  if (configuration.has_invalid_switch()) {
    return ProcessExitResult(INVALID_OPTION);
  }

  // Show the progress window.
  ProgressState progress_state;
  HWND progress_hwnd = CreateProgressWindow(module, &progress_state);
  SetProgress(progress_hwnd, 5, L"Preparing installation directory\x2026");

  // First get a path where we can extract payload
  PathString base_path;
  if (!GetWorkDir(module, &base_path, &exit_code)) {
    if (progress_hwnd) {
      ::DestroyWindow(progress_hwnd);
    }
    return exit_code;
  }

  SetProgress(progress_hwnd, 15, L"Extracting files\x2026");

  int max_delete_attempts = 0;
  PathString setup_path;
  PathString archive_path;
  ResourceTypeString archive_type;

  exit_code =
      UnpackBinaryResources(module, base_path.get(), setup_path, archive_path,
                            archive_type, max_delete_attempts);

  // While unpacking the binaries, we paged in a whole bunch of memory that
  // we don't need anymore.  Let's give it back to the pool before running
  // setup.
  ::SetProcessWorkingSetSize(::GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);

  if (exit_code.IsSuccess()) {
    // Switch to marquee — we can't measure setup.exe's internal progress.
    SetProgress(progress_hwnd, 40, L"Installing Neovex\x2026");
    SetMarquee(progress_hwnd, L"Installing \x2014 this may take a moment\x2026");

    exit_code = RunSetup(configuration, archive_path.get(), setup_path.get(),
                         archive_type.compare(kLZMAResourceType) == 0);
  }

  // Show completion before tearing down.
  if (exit_code.IsSuccess()) {
    SetProgress(progress_hwnd, 100, L"Installation Complete!");
    
    if (progress_hwnd) {
      ProgressState* ps = reinterpret_cast<ProgressState*>(
          ::GetWindowLongPtr(progress_hwnd, GWLP_USERDATA));
      if (ps) {
        ps->is_complete = true;
        ::InvalidateRect(progress_hwnd, nullptr, FALSE);
      }
      
      MSG msg;
      while (::GetMessage(&msg, nullptr, 0, 0)) {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
      }
      progress_hwnd = nullptr;
    }
  }

  if (progress_hwnd && ::IsWindow(progress_hwnd)) {
    ::DestroyWindow(progress_hwnd);
  }

  if (configuration.should_delete_extracted_files()) {
    DeleteExtractedFiles(module, archive_path, setup_path, base_path,
                         max_delete_attempts);
  }

#if BUILDFLAG(GOOGLE_CHROME_BRANDING)
  if (exit_code.IsSuccess()) {
    // Send up a signal in ExtraCode1 upon successful install indicating the
    // maximum number of retries needed to delete a file or directory by
    // DeleteWithRetry; see https://crbug.com/1138157.
    MetricSample max_retries =
        (max_delete_attempts > 1 ? max_delete_attempts - 1 : 0);
    WriteExtraCode1(configuration,
                    MetricToExtraCode1(kMaxDeleteRetryCount, max_retries));
  } else {
    WriteInstallResults(configuration, exit_code);
  }
#endif

  return exit_code;
}

}  // namespace mini_installer
