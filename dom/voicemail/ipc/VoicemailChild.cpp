/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "VoicemailChild.h"
#include "mozilla/Services.h"
#include "mozilla/dom/ContentChild.h"
#include "VoicemailStatus.h"

#include <android/log.h>
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "VoicemailChild", args);

using namespace mozilla::dom::voicemail;

namespace {

already_AddRefed<nsIDOMMozVoicemailStatus>
CreateStatusFromStatusData(const VoicemailStatusData& aData)
{
  nsCOMPtr<nsIDOMMozVoicemailStatus> status;
  status = new VoicemailStatus(aData);

  return status.forget();
}

} // anonymous namespace

VoicemailChild::VoicemailChild(nsIVoicemailListener* aListener)
  : mListener(aListener)
{
  MOZ_COUNT_CTOR(VoicemailChild);
}

VoicemailChild::~VoicemailChild()
{
  MOZ_COUNT_DTOR(VoicemailChild);
}

void
VoicemailChild::ActorDestroy(ActorDestroyReason aWhy)
{
}

bool
VoicemailChild::RecvNotifyStatusChanged(const VoicemailStatusData& aData)
//nsIDOMMozVoicemailStatus* const& aStatus)
{
  LOG("XXX RecvNotifyStatusChanged");
  nsCOMPtr<nsIDOMMozVoicemailStatus> status =
    CreateStatusFromStatusData(aData);
  mListener->NotifyStatusChanged(status);//aStatus);
  return true;
}
