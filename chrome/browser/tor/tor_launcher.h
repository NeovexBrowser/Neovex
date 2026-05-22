// Copyright 2024 Neovex Authors. All rights reserved.
// NEOVEX TOR

#ifndef CHROME_BROWSER_TOR_TOR_LAUNCHER_H_
#define CHROME_BROWSER_TOR_TOR_LAUNCHER_H_

#include <string>

#include "base/files/file_path.h"
#include "base/process/process.h"

namespace neovex {

// TorLauncher manages the lifecycle of the Tor SOCKS5 proxy process.
// It is a singleton owned by TorLauncherFactory.
class TorLauncher {
 public:
  TorLauncher();
  ~TorLauncher();

  TorLauncher(const TorLauncher&) = delete;
  TorLauncher& operator=(const TorLauncher&) = delete;

  // Start the Tor process. Returns true if launch succeeded.
  bool Start();

  // Stop the Tor process.
  void Stop();

  // Returns true if the Tor process is currently running.
  bool IsRunning() const;

  // NEOVEX TOR - Check if Tor SOCKS proxy is ready to accept connections.
  // This probes the SOCKS port to verify the proxy is actually listening.
  bool IsReady() const;

  // NEOVEX TOR - Get the real logs from the Tor process
  std::string GetLogs() const;

 private:
  // The Tor child process.
  base::Process tor_process_;

  // Whether Tor has been started.
  bool is_started_ = false;

  // NEOVEX TOR - Whether Tor SOCKS proxy is confirmed ready
  bool is_ready_ = false;

  // NEOVEX TOR - The file path to the Tor log file
  base::FilePath tor_log_file_;
};

}  // namespace neovex

#endif  // CHROME_BROWSER_TOR_TOR_LAUNCHER_H_
