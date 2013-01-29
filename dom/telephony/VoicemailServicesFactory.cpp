/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "VoicemailServicesFactory.h"
#include "nsXULAppAPI.h"
#include "VoicemailService.h"
#include "VoicemailIPCService.h"
#include "nsServiceManagerUtils.h"


namespace mozilla {
namespace dom {
namespace telephony {

/* static */ already_AddRefed<nsIVoicemailService>
VoicemailServicesFactory::CreateVoicemailService()
{
  nsCOMPtr<nsIVoicemailService> voicemailService;

  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    voicemailService = new VoicemailIPCService();
  } else {
    voicemailService = new VoicemailService();
  }

  return voicemailService.forget();
}

} // namespace telephony
} // namespace dom
} // namespace mozilla
