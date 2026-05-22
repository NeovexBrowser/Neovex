// Copyright 2024 Neovex Authors. All rights reserved.
// NEOVEX TOR

#include "chrome/browser/ui/webui/ntp/tor_status_handler.h"

#include "base/functional/bind.h"
#include "base/values.h"
#include "chrome/browser/tor/tor_launcher_factory.h"

TorStatusHandler::TorStatusHandler() = default;

TorStatusHandler::~TorStatusHandler() {
  poll_timer_.Stop();
}

void TorStatusHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "getTorStatus",
      base::BindRepeating(&TorStatusHandler::HandleGetTorStatus,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "enableTor",
      base::BindRepeating(&TorStatusHandler::HandleEnableTor,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "disableTor",
      base::BindRepeating(&TorStatusHandler::HandleDisableTor,
                          base::Unretained(this)));
}

void TorStatusHandler::HandleEnableTor(const base::ListValue& args) {
  AllowJavascript();

  neovex::TorLauncherFactory* factory =
      neovex::TorLauncherFactory::GetInstance();
  factory->OnIncognitoWindowOpened();

  was_ready_ = false;

  // Start polling for status
  poll_timer_.Start(FROM_HERE, base::Seconds(2),
                    base::BindRepeating(&TorStatusHandler::PollTorStatus,
                                        base::Unretained(this)));
}

void TorStatusHandler::HandleDisableTor(const base::ListValue& args) {
  AllowJavascript();

  poll_timer_.Stop();
  was_ready_ = false;

  neovex::TorLauncherFactory* factory =
      neovex::TorLauncherFactory::GetInstance();
  factory->OnIncognitoWindowClosed();
}

void TorStatusHandler::HandleGetTorStatus(const base::ListValue& args) {
  AllowJavascript();

  // Send current status
  SendTorStatus();

  // If not ready yet, start polling
  if (!was_ready_) {
    poll_timer_.Start(FROM_HERE, base::Seconds(2),
                      base::BindRepeating(&TorStatusHandler::PollTorStatus,
                                          base::Unretained(this)));
  }
}

void TorStatusHandler::PollTorStatus() {
  SendTorStatus();

  // Stop polling once Tor is ready
  if (was_ready_) {
    poll_timer_.Stop();
  }
}

void TorStatusHandler::SendTorStatus() {
  neovex::TorLauncherFactory* factory =
      neovex::TorLauncherFactory::GetInstance();
  neovex::TorLauncher* launcher = factory->GetLauncher();

  base::DictValue tor_status;

  if (launcher) {
    bool is_ready = launcher->IsReady();
    tor_status.Set("isReady", is_ready);
    tor_status.Set("isRunning", launcher->IsRunning());
    tor_status.Set("logs", launcher->GetLogs());

    if (is_ready) {
      was_ready_ = true;
    }
  } else {
    tor_status.Set("isReady", false);
    tor_status.Set("isRunning", false);
    tor_status.Set("logs", "Waiting for Tor to start...");
  }

  FireWebUIListener("tor-status-changed", tor_status);
}
