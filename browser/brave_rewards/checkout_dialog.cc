/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/checkout_dialog.h"

#include <memory>
#include <vector>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/browser/brave_rewards/checkout_dialog_message_handler.h"
#include "brave/common/webui_url_constants.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "components/constrained_window/constrained_window_views.h"
#include "components/web_modal/web_contents_modal_dialog_host.h"
#include "content/public/browser/web_contents.h"
#include "ui/web_dialogs/web_dialog_delegate.h"

using content::WebContents;
using content::WebUIMessageHandler;

namespace {

constexpr int kDialogMargin = 25;

constexpr int kDialogMinWidth = 548;
constexpr int kDialogMinHeight = 200;

constexpr int kDialogMaxWidth = 548;
constexpr int kDialogMaxHeight = 800;

// Returns the maximum dialog size for the specified dialog
// initiator WebContents. The dialog cannot be larger than
// the tab contents to which it applies.
gfx::Size GetMaxDialogSize(WebContents* initiator) {
  WebContents* top_level =
      constrained_window::GetTopLevelWebContents(initiator);
  gfx::Size size;
  if (auto* browser = chrome::FindBrowserWithWebContents(top_level)) {
    if (auto* host = browser->window()->GetWebContentsModalDialogHost()) {
      size = host->GetMaximumDialogSize();
    }
  }
  if (size.IsEmpty()) {
    size = top_level->GetContainerBounds().size();
  }
  size -= gfx::Size(kDialogMargin, 0);
  size.SetToMin(gfx::Size(kDialogMaxWidth, kDialogMaxHeight));
  return size;
}

using brave_rewards::CheckoutDialogClosedCallback;
using brave_rewards::CheckoutDialogController;
using brave_rewards::CheckoutDialogMessageHandler;

class CheckoutDialogDelegate : public ui::WebDialogDelegate {
 public:
  CheckoutDialogDelegate(
      base::Value params,
      CheckoutDialogClosedCallback closed_callback)
      : params_(std::move(params)),
        closed_callback_(std::move(closed_callback)) {}

  CheckoutDialogDelegate(const CheckoutDialogDelegate&) = delete;
  CheckoutDialogDelegate& operator=(const CheckoutDialogDelegate&) = delete;

  ~CheckoutDialogDelegate() override {}

  ui::ModalType GetDialogModalType() const override {
    return ui::MODAL_TYPE_CHILD;
  }

  base::string16 GetDialogTitle() const override {
    return base::string16();
  }

  GURL GetDialogContentURL() const override {
    return GURL(kBraveUICheckoutURL);
  }

  void GetWebUIMessageHandlers(
      std::vector<WebUIMessageHandler*>* handlers) const override {
    // Handlers are added in OnDialogShown
  }

  void GetDialogSize(gfx::Size* size) const override {
    // We don't need to set the dialog size here as auto-resize
    // is enabled.
  }

  std::string GetDialogArgs() const override {
    std::string json;
    base::JSONWriter::Write(params_, &json);
    return json;
  }

  void OnDialogShown(content::WebUI* webui) override {
    webui->AddMessageHandler(
        std::make_unique<CheckoutDialogMessageHandler>(&controller_));
  }

  void OnDialogClosed(const std::string& json_retval) override {
    if (closed_callback_) {
      std::move(closed_callback_).Run();
    }
  }

  void OnCloseContents(
      WebContents* source,
      bool* out_close_dialog) override {
    *out_close_dialog = true;
  }

  bool ShouldShowDialogTitle() const override {
    return false;
  }

  base::WeakPtr<CheckoutDialogController> GetController() {
    return controller_.AsWeakPtr();
  }

 private:
  base::Value params_;
  CheckoutDialogClosedCallback closed_callback_;
  CheckoutDialogController controller_;
};

}  // namespace

namespace brave_rewards {

base::WeakPtr<CheckoutDialogController> ShowCheckoutDialog(
    WebContents* initiator,
    CheckoutDialogClosedCallback on_dialog_closed) {
  // TODO(zenparsing): Take params from caller
  base::Value order_info(base::Value::Type::DICTIONARY);
  order_info.SetStringKey("description", "Some order description");
  order_info.SetDoubleKey("total", 15.0);

  base::Value params(base::Value::Type::DICTIONARY);
  params.SetKey("orderInfo", std::move(order_info));

  auto delegate = std::make_unique<CheckoutDialogDelegate>(
      std::move(params),
      std::move(on_dialog_closed));

  auto controller = delegate->GetController();

  gfx::Size min_size(kDialogMinWidth, kDialogMinHeight);
  gfx::Size max_size = GetMaxDialogSize(initiator);

  ShowConstrainedWebDialogWithAutoResize(
      initiator->GetBrowserContext(),
      std::move(delegate),
      initiator,
      min_size,
      max_size);

  return controller;
}

}  // namespace brave_rewards
