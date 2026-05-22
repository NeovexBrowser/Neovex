// Copyright 2024 Neovex Authors. All rights reserved.
// NEOVEX TOR

#ifndef CHROME_BROWSER_UI_WEBUI_NTP_TOR_STATUS_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_NTP_TOR_STATUS_HANDLER_H_

#include "base/timer/timer.h"
#include "base/values.h"
#include "content/public/browser/web_ui_message_handler.h"

// Handles Tor status queries and enable/disable from the incognito NTP page.
class TorStatusHandler : public content::WebUIMessageHandler {
 public:
  TorStatusHandler();
  ~TorStatusHandler() override;

  TorStatusHandler(const TorStatusHandler&) = delete;
  TorStatusHandler& operator=(const TorStatusHandler&) = delete;

  // WebUIMessageHandler:
  void RegisterMessages() override;

 private:
  void HandleGetTorStatus(const base::ListValue& args);
  void HandleEnableTor(const base::ListValue& args);
  void HandleDisableTor(const base::ListValue& args);
  void PollTorStatus();
  void SendTorStatus();

  base::RepeatingTimer poll_timer_;
  bool was_ready_ = false;
};

#endif  // CHROME_BROWSER_UI_WEBUI_NTP_TOR_STATUS_HANDLER_H_
