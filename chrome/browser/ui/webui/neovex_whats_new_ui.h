// Copyright 2026 Neovex Authors
// Neovex What's New page UI.

#ifndef CHROME_BROWSER_UI_WEBUI_NEOVEX_WHATS_NEW_UI_H_
#define CHROME_BROWSER_UI_WEBUI_NEOVEX_WHATS_NEW_UI_H_

#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/webui_config.h"

class NeovexWhatsNewUI;

class NeovexWhatsNewUIConfig : public content::DefaultWebUIConfig<NeovexWhatsNewUI> {
 public:
  NeovexWhatsNewUIConfig();
  ~NeovexWhatsNewUIConfig() override = default;
};

class NeovexWhatsNewUI : public content::WebUIController {
 public:
  explicit NeovexWhatsNewUI(content::WebUI* web_ui);
  NeovexWhatsNewUI(const NeovexWhatsNewUI&) = delete;
  NeovexWhatsNewUI& operator=(const NeovexWhatsNewUI&) = delete;
  ~NeovexWhatsNewUI() override;
};

#endif  // CHROME_BROWSER_UI_WEBUI_NEOVEX_WHATS_NEW_UI_H_
