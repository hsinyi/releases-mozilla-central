/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/ContentChild.h"
#include "VoicemailIPCService.h"
#include "nsXULAppAPI.h"
#include "jsapi.h"
#include "VoicemailChild.h"
#include "mozilla/dom/voicemail/PVoicemailChild.h"
#include "nsIDOMMozVoicemailStatus.h"

#include <android/log.h>
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "VoicemailIPCSevice", args); 

namespace mozilla {
namespace dom {
namespace voicemail {

NS_IMPL_ISUPPORTS2(VoicemailIPCService,
                   nsIVoicemailProvider,
                   nsIVoicemailListener)

VoicemailIPCService::VoicemailIPCService()
{
  mChild = new VoicemailChild(this);
}

VoicemailIPCService::~VoicemailIPCService()
{
  if (mChild) {
    ReleaseIPCChild();
  }
}

void
VoicemailIPCService::ReleaseIPCChild()
{
  PVoicemailChild::Send__delete__(mChild);
  mChild = nullptr;
}

/*
 * Implementation of nsIVoicemailProvider
 */
NS_IMETHODIMP
VoicemailIPCService::Register(nsIVoicemailListener* aListener)
{

  NS_ENSURE_STATE(!mListeners.Contains(aListener));
  NS_ENSURE_TRUE(mListeners.AppendElement(aListener), NS_ERROR_FAILURE);

  if (mChild) {//!mChild) {
    mChild = ContentChild::GetSingleton()->SendPVoicemailConstructor(mChild);
    LOG("XXX SendPVoicemailConstructor: mChild: %p\n", mChild);
    if (!mChild) {
      LOG("XXX SendPVoicemailConstructor fails");
      return NS_OK;
    }
  }

  return NS_OK; 
}

NS_IMETHODIMP
VoicemailIPCService::Unregister(nsIVoicemailListener* aListener)
{
 NS_ENSURE_TRUE(mListeners.RemoveElement(aListener), NS_ERROR_FAILURE);

 if (mListeners.IsEmpty()) {
    ReleaseIPCChild();
  }

  return NS_OK;
} 

NS_IMETHODIMP
VoicemailIPCService::GetNumber(nsAString& aNumber)
{
  if (!mChild) {
    return NS_OK;
  }

  nsString number;
  mChild->SendGetNumber(&number);
  aNumber.Assign(number);
  return NS_OK;
}

NS_IMETHODIMP
VoicemailIPCService::GetDisplayName(nsAString& aDisplayName)
{
  if (!mChild) {
    return NS_OK;
  }

  nsString displayName;
  mChild->SendGetDisplayName(&displayName);
  aDisplayName.Assign(displayName);
  return NS_OK;
}

// nsIVoicemailListener

NS_IMETHODIMP
VoicemailIPCService::NotifyStatusChanged(nsIDOMMozVoicemailStatus* aStatus)
{
  LOG("XXX NotifyStatusChanged");
  const uint32_t length = mListeners.Length();
  for (uint32_t index = 0; index < length; index++) {
    mListeners[index]->NotifyStatusChanged(aStatus);
  }
  return NS_OK;
}

} // namespace voicemail
} // namespace dom
} // namespace mozilla
