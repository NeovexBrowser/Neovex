// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(crbug.com/40285824): Remove this and convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include "chrome/installer/mini_installer/write_to_disk.h"

#include <windows.h>

#include <stddef.h>

#include <algorithm>

#include "chrome/installer/mini_installer/memory_range.h"
#include "chrome/installer/mini_installer/mini_file.h"

namespace mini_installer {

bool WriteToDisk(const MemoryRange& data, const wchar_t* full_path) {
  MiniFile file;

  // Retry file creation to handle transient locks from antivirus scanners,
  // leftover handles from a previous installer cleanup, or file system delays
  // in finalizing a DeleteOnClose.  This is the root cause of "exit code 57"
  // and similar intermittent extraction failures.
  constexpr int kMaxCreateAttempts = 20;
  constexpr DWORD kCreateRetryDelayMs = 100;
  bool created = false;
  for (int attempt = 0; attempt < kMaxCreateAttempts; ++attempt) {
    if (file.Create(full_path)) {
      created = true;
      break;
    }
    DWORD err = ::GetLastError();
    // Only retry on transient errors.
    if (err != ERROR_SHARING_VIOLATION &&
        err != ERROR_ACCESS_DENIED &&
        err != ERROR_LOCK_VIOLATION &&
        err != ERROR_NOT_READY) {
      break;
    }
    ::Sleep(kCreateRetryDelayMs);
  }
  if (!created) {
    return false;
  }

  // Don't write all of the data at once because this can lead to kernel
  // address-space exhaustion on 32-bit Windows (see https://crbug.com/1001022
  // for details).
  constexpr size_t kMaxWriteAmount = 8 * 1024 * 1024;
  for (size_t total_written = 0; total_written < data.size; /**/) {
    const size_t write_amount =
        std::min(kMaxWriteAmount, data.size - total_written);
    DWORD written = 0;
    if (!::WriteFile(file.GetHandleUnsafe(), data.data + total_written,
                     static_cast<DWORD>(write_amount), &written, nullptr)) {
      const auto write_error = ::GetLastError();

      // Delete the file since the write failed.
      file.DeleteOnClose();
      file.Close();

      ::SetLastError(write_error);
      return false;
    }
    total_written += write_amount;
  }

  // Flush file buffers to ensure the data is fully committed to disk before
  // the handle is closed.  This prevents corruption on some Windows 10
  // configurations (see https://crbug.com/1443320 for similar issue in
  // decompress.cc).
  ::FlushFileBuffers(file.GetHandleUnsafe());

  return true;
}

}  // namespace mini_installer
