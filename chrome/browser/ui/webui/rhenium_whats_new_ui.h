// Copyright 2026 Rhenium Authors
// Rhenium What's New page UI.

#ifndef CHROME_BROWSER_UI_WEBUI_RHENIUM_WHATS_NEW_UI_H_
#define CHROME_BROWSER_UI_WEBUI_RHENIUM_WHATS_NEW_UI_H_

#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/webui_config.h"

class RheniumWhatsNewUI;

class RheniumWhatsNewUIConfig : public content::DefaultWebUIConfig<RheniumWhatsNewUI> {
 public:
  RheniumWhatsNewUIConfig();
  ~RheniumWhatsNewUIConfig() override = default;
};

class RheniumWhatsNewUI : public content::WebUIController {
 public:
  explicit RheniumWhatsNewUI(content::WebUI* web_ui);
  RheniumWhatsNewUI(const RheniumWhatsNewUI&) = delete;
  RheniumWhatsNewUI& operator=(const RheniumWhatsNewUI&) = delete;
  ~RheniumWhatsNewUI() override;
};

#endif  // CHROME_BROWSER_UI_WEBUI_RHENIUM_WHATS_NEW_UI_H_
