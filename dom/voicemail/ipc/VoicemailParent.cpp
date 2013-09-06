/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "VoicemailParent.h"
#include "mozilla/Services.h"
#include "mozilla/unused.h"
#include "VoicemailStatus.h"

#include <android/log.h>
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "VoicemailParent", args); 

namespace mozilla {
namespace dom {
namespace voicemail {

NS_IMPL_ISUPPORTS1(VoicemailParent, nsIVoicemailListener)

void
VoicemailParent::ActorDestroy(ActorDestroyReason why)
{
  mActorDestroyed = true;
  nsCOMPtr<nsIVoicemailProvider> provider =
    do_GetService(VOICEMAIL_SERVICE_CONTRACTID);
  provider->Unregister(this); 
}

bool
VoicemailParent::RegisterListener()
{
  LOG("XXX RecvPVoicemailConstructor - register");
  nsCOMPtr<nsIVoicemailProvider> provider =
    do_GetService(VOICEMAIL_SERVICE_CONTRACTID);
  provider->Register(this);

  return true;
}

bool
VoicemailParent::RecvGetNumber(nsString* aNumber)
{
  nsCOMPtr<nsIVoicemailProvider> provider =
    do_GetService(VOICEMAIL_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(provider, true);

  nsString number;
  provider->GetNumber(number);
  *aNumber = number;
  return true;
}

bool
VoicemailParent::RecvGetDisplayName(nsString* aDisplayName)
{
  nsCOMPtr<nsIVoicemailProvider> provider =
    do_GetService(VOICEMAIL_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(provider, true);

  nsString displayName;
  provider->GetDisplayName(displayName);
  *aDisplayName = displayName;
  return true;
}


bool
VoicemailParent::GetVoicemailStatusDataFromStatus(nsIDOMMozVoicemailStatus *aStatus,
                                                  VoicemailStatusData &aData)
{
  LOG("XXX GetVoicemailStatusDataFromStatus");
  aData = static_cast<VoicemailStatus*>(aStatus)->GetData();
  return true;
}


// nsIVoicemailListener

NS_IMETHODIMP
VoicemailParent::NotifyStatusChanged(nsIDOMMozVoicemailStatus* aStatus)
{
  LOG("XXX SendNotifyStatusChanged");
  // The child process could die before this asynchronous notification, in which
  // case ActorDestroy() was called and mActorDestroyed is set to true. Return
  // an error here to avoid sending a message to the dead process.
  NS_ENSURE_TRUE(!mActorDestroyed, NS_ERROR_FAILURE);
  LOG("XXX 1");

  VoicemailStatusData statusData;
  if (!GetVoicemailStatusDataFromStatus(aStatus, statusData)) {
    LOG("XXX 4");
    NS_ERROR("Got a voicemailstatuschanged notification without a valid status.");
    return NS_OK;
  }
  LOG("XXX 2");
  SendNotifyStatusChanged(statusData);
  LOG("XXX 3");
  return NS_OK;
}

}
}
}
