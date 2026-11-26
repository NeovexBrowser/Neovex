// Copyright 2026 Rhenium Authors
// Rhenium What's New page UI implementation.

#include "chrome/browser/ui/webui/rhenium_whats_new_ui.h"

#include "base/containers/span.h"
#include "chrome/browser/rhenium_version.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/grit/rhenium_whats_new_resources.h"
#include "chrome/grit/rhenium_whats_new_resources_map.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/webui/webui_util.h"

RheniumWhatsNewUIConfig::RheniumWhatsNewUIConfig()
    : DefaultWebUIConfig(content::kChromeUIScheme,
                         chrome::kChromeUIRheniumWhatsNewHost) {}

RheniumWhatsNewUI::RheniumWhatsNewUI(content::WebUI* web_ui)
    : content::WebUIController(web_ui) {
  content::WebUIDataSource* source = content::WebUIDataSource::CreateAndAdd(
      Profile::FromWebUI(web_ui), chrome::kChromeUIRheniumWhatsNewHost);

  // Add the current version so JS can display it.
  source->AddString("rheniumVersion", RHENIUM_VERSION);

  // Setup resources using the standard WebUI utility. This correctly configures
  // Content Security Policies, TrustedTypes, and string injection, preventing
  // KILLED_BAD_MESSAGE renderer crashes.
  webui::SetupWebUIDataSource(
      source, base::span<const webui::ResourcePath>(kRheniumWhatsNewResources),
      IDR_RHENIUM_WHATS_NEW_RHENIUM_WHATS_NEW_HTML);
}

RheniumWhatsNewUI::~RheniumWhatsNewUI() = default;
