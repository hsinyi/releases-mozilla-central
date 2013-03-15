/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "nsIDOMVoicemailStatus.h"

#include "mozilla/dom/ContentChild.h"
#include "VoicemailIPCService.h"
#include "nsXULAppAPI.h"
#include "jsapi.h"
#include "VoicemailChild.h"
#include "mozilla/dom/telephony/PVoicemailChild.h"

#include <android/log.h>
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "voicemailIPCService", args);

namespace mozilla {
namespace dom {
namespace telephony {

PVoicemailChild* gVoicemailChild;

NS_IMPL_ISUPPORTS1(VoicemailIPCService, nsIVoicemailService)

PVoicemailChild*
VoicemailIPCService::GetVoicemailChild()
{
  if (!gVoicemailChild) {
    gVoicemailChild = ContentChild::GetSingleton()->SendPVoicemailConstructor();
  }

  return gVoicemailChild;
}

/*
 * Implementation of nsIVoicemailService
 */
NS_IMETHODIMP
VoicemailIPCService::TestRegister(int32_t aId, int32_t* aReturnedId)
{
  GetVoicemailChild()->SendTestRegister(aId, aReturnedId);

  return NS_OK;
}

NS_IMETHODIMP
VoicemailIPCService::GetNumber(nsAString& aNumber)
{
  nsString number;
  GetVoicemailChild()->SendGetNumber(&number);
  aNumber.Assign(number);
  return NS_OK;
}

NS_IMETHODIMP
VoicemailIPCService::GetDisplayName(nsAString& aDisplayName)
{
  nsString displayName;
  GetVoicemailChild()->SendGetDisplayName(&displayName);
  aDisplayName.Assign(displayName);
  return NS_OK;
}

} // namespace telephony
} // namespace dom
} // namespace mozilla
