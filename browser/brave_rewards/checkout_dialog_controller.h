/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_REWARDS_CHECKOUT_DIALOG_CONTROLLER_H_
#define BRAVE_BROWSER_BRAVE_REWARDS_CHECKOUT_DIALOG_CONTROLLER_H_

#include <utility>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"

namespace brave_rewards {

// Provides the ability to send messages to the checkout dialog
// from the component that opened the dialog. For example, when
// an order has been aborted by a merchant website, an Abort()
// message can be sent to cancel the payment and close the dialog.
class CheckoutDialogController :
    public base::SupportsWeakPtr<CheckoutDialogController> {

 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnAbort() = 0;
    virtual void OnComplete() = 0;
  };

  CheckoutDialogController();
  ~CheckoutDialogController();

  CheckoutDialogController(const CheckoutDialogController&) = delete;
  CheckoutDialogController& operator=(
      const CheckoutDialogController&) = delete;

  void Abort();
  void Complete();

 private:
  friend class CheckoutDialogMessageHandler;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  base::ObserverList<Observer> observers_;
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_BRAVE_REWARDS_CHECKOUT_DIALOG_CONTROLLER_H_
