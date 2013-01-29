
/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "VoicemailService.h"
#include "SystemWorkerManager.h"
#include "nsIInterfaceRequestorUtils.h"

using mozilla::dom::gonk::SystemWorkerManager;

namespace mozilla {
namespace dom {
namespace telephony {

NS_IMPL_ISUPPORTS1(VoicemailService, nsIVoicemailService)

VoicemailService::VoicemailService()
{
  nsIInterfaceRequestor* ireq = SystemWorkerManager::GetInterfaceRequestor();
  NS_WARN_IF_FALSE(ireq, "The SystemWorkerManager has not been created!");

  if (ireq) {
    mRIL = do_GetInterface(ireq);
    NS_WARN_IF_FALSE(mRIL, "This shouldn't fail!");
  }
}

NS_IMETHODIMP
VoicemailService::TestRegister(int32_t aId, int32_t* aReturnedId)
{
  if (!mRIL) {
    return NS_OK;
  }
  mRIL->TestRegister(aId, aReturnedId);
  return NS_OK;
}

} // namespace telephony
} // namespace dom
} // namespace mozilla
