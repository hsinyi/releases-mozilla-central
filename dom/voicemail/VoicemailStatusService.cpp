/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "VoicemailStatus.h"
#include "VoicemailStatusService.h"
#include "jsapi.h"

namespace mozilla {
namespace dom {
namespace voicemail {

NS_IMPL_ISUPPORTS1(VoicemailStatusService, nsIVoicemailStatusService)

/* static */ StaticRefPtr<VoicemailStatusService> VoicemailStatusService::sSingleton;

/* static */ already_AddRefed<VoicemailStatusService>
VoicemailStatusService::GetInstance()
{
  if (!sSingleton) {
    sSingleton = new VoicemailStatusService();
    ClearOnShutdown(&sSingleton);
  }

  nsRefPtr<VoicemailStatusService> service = sSingleton.get();
  return service.forget();
}

NS_IMETHODIMP
VoicemailStatusService::CreateStatus(bool aHasMessages,
                                     int32_t aMessageCount,
                                     const nsAString& aReturnNumber,
                                     const nsAString& aReturnMessage,
                                     JSContext* aCx,
                                     nsIDOMMozVoicemailStatus** aStatus)
{
  return VoicemailStatus::Create(aHasMessages,
                                 aMessageCount,
                                 aReturnNumber,
                                 aReturnMessage,
                                 aCx,
                                 aStatus);
}

} // namespace voicemail
} // namespace dom
} // namespace mozilla
