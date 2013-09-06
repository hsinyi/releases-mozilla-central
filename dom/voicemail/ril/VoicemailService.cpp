/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "VoicemailService.h"
#include "mozilla/Services.h"
#include "nsServiceManagerUtils.h"

#include <android/log.h>
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "VoicemailSevice", args);

namespace mozilla {
namespace dom {
namespace voicemail {

VoicemailService::VoicemailService()
{
  mRIL = do_GetService("@mozilla.org/ril;1");

  NS_WARN_IF_FALSE(mRIL, "This shouldn't fail!");
}


VoicemailService::~VoicemailService()
{
  mRIL = nullptr;
}

NS_IMPL_ISUPPORTS1(VoicemailService, nsIVoicemailProvider)

NS_IMETHODIMP
VoicemailService::Register(nsIVoicemailListener* aListener)
{
  NS_ENSURE_TRUE(mRIL, NS_ERROR_FAILURE);

  LOG("XXX register");
  mRIL->RegisterVoicemailListener(aListener);
  return NS_OK;
}

NS_IMETHODIMP
VoicemailService::Unregister(nsIVoicemailListener* aListener)
{
  mRIL->UnregisterVoicemailListener(aListener);
  return NS_OK;
}

NS_IMETHODIMP
VoicemailService::GetNumber(nsAString& aNumber)
{
  if (!mRIL) {
    return NS_OK;
  }

  return mRIL->GetVoicemailNumber(aNumber);
}

NS_IMETHODIMP
VoicemailService::GetDisplayName(nsAString& aDisplayName)
{
  if (!mRIL) {
    return NS_OK;
  }

  return mRIL->GetVoicemailDisplayName(aDisplayName);
}
} // namespace voicemail
} // namespace dom
} // namespace mozilla
