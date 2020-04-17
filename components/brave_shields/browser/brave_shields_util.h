/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_BRAVE_SHIELDS_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_BRAVE_SHIELDS_UTIL_H_

#include <stdint.h>
#include <string>

#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "services/network/public/mojom/referrer_policy.mojom.h"

namespace content {
class BrowserContext;
struct Referrer;
}

class GURL;
class HostContentSettingsMap;

namespace brave_shields {

enum ControlType { ALLOW = 0, BLOCK, BLOCK_THIRD_PARTY, DEFAULT, INVALID };

ContentSettingsPattern GetPatternFromURL(const GURL& url);
std::string ControlTypeToString(ControlType type);
ControlType ControlTypeFromString(const std::string& string);

void SetBraveShieldsEnabled(content::BrowserContext* browser_context,
                            bool enable,
                            const GURL& url);
// reset to the default value
void ResetBraveShieldsEnabled(content::BrowserContext* browser_context,
                              const GURL& url);
bool GetBraveShieldsEnabled(content::BrowserContext* browser_context,
                            const GURL& url);
bool GetBraveShieldsEnabled(HostContentSettingsMap* map, const GURL& url);

void SetAdControlType(content::BrowserContext* browser_context,
                      ControlType type,
                      const GURL& url);
ControlType GetAdControlType(content::BrowserContext* browser_context,
                             const GURL& url);

void SetCookieControlType(content::BrowserContext* browser_context,
                          ControlType type,
                          const GURL& url);
void SetCookieControlType(HostContentSettingsMap* map,
                          ControlType type,
                          const GURL& url);
ControlType GetCookieControlType(content::BrowserContext* browser_context,
                                 const GURL& url);
ControlType GetCookieControlType(HostContentSettingsMap* map, const GURL& url);

// Referrers is always set along with cookies so there is no setter and
// these is just included for backwards compat.
bool AllowReferrers(content::BrowserContext* browser_context, const GURL& url);
bool AllowReferrers(HostContentSettingsMap* map, const GURL& url);

void SetFingerprintingControlType(content::BrowserContext* browser_context,
                                  ControlType type,
                                  const GURL& url);
ControlType GetFingerprintingControlType(
    content::BrowserContext* browser_context,
    const GURL& url);

void SetHTTPSEverywhereEnabled(content::BrowserContext* browser_context,
                               bool enable,
                               const GURL& url);
// reset to the default value
void SetHTTPSEverywhereEnabled(content::BrowserContext* browser_context,
                               bool enable,
                               const GURL& url);
void ResetHTTPSEverywhereEnabled(content::BrowserContext* browser_context,
                                 const GURL& url);
bool GetHTTPSEverywhereEnabled(content::BrowserContext* browser_context,
                               const GURL& url);

void SetNoScriptControlType(content::BrowserContext* browser_context,
                            ControlType type,
                            const GURL& url);
ControlType GetNoScriptControlType(content::BrowserContext* browser_context,
                                   const GURL& url);

void DispatchBlockedEvent(const GURL& request_url,
                          int render_frame_id,
                          int render_process_id,
                          int frame_tree_node_id,
                          const std::string& block_type);

bool ShouldSetReferrer(bool allow_referrers,
                       bool shields_up,
                       const GURL& original_referrer,
                       const GURL& tab_origin,
                       const GURL& target_url,
                       const GURL& new_referrer_url,
                       network::mojom::ReferrerPolicy policy,
                       content::Referrer* output_referrer);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_BRAVE_SHIELDS_UTIL_H_
