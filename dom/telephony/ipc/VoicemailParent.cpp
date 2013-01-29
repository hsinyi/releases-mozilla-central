/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "VoicemailParent.h"
#include "nsIVoicemailService.h"
#include "mozilla/Services.h"
#include "mozilla/unused.h"

namespace mozilla {
namespace dom {
namespace telephony {

VoicemailParent::VoicemailParent()
{
  MOZ_COUNT_CTOR(VoicemailParent);
}

VoicemailParent::~VoicemailParent()
{
  MOZ_COUNT_DTOR(VoicemailParent);
}

void
VoicemailParent::ActorDestroy(ActorDestroyReason why)
{
}

bool
VoicemailParent::RecvTestRegister(const int32_t& aId,
                                  int32_t* aReturnId)
{
  nsCOMPtr<nsIVoicemailService> voicemailService =
    do_GetService(VOICEMAIL_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(voicemailService, true);

  voicemailService->TestRegister(aId, aReturnId);
  return true;
}

}
}
}
