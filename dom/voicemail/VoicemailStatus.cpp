/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "VoicemailStatus.h"
#include "nsDOMClassInfo.h"
#include "mozilla/dom/voicemail/VoicemailTypes.h"

#include <android/log.h>
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "VoicemailStatus", args); 

using namespace mozilla::dom::voicemail;

VoicemailStatus::VoicemailStatus(bool aHasMessages,
                                 int32_t aMessageCount,
                                 const nsString& aReturnNumber,
                                 const nsString& aReturnMessage)
  : mData(aHasMessages, aMessageCount, aReturnNumber, aReturnMessage)
//  mHasMessages(aHasMessages)
//  , mMessageCount(aMessageCount)
//  , mReturnNumber(aReturnNumber)
//  , mReturnMessage(aReturnMessage)
{
}

VoicemailStatus::VoicemailStatus(const VoicemailStatusData& aData)
  : mData(aData)
{
}

/* static */ nsresult
VoicemailStatus::Create(bool aHasMessages,
                        int32_t aMessageCount,
                        const nsAString& aReturnNumber,
                        const nsAString& aReturnMessage,
                        JSContext* aCx,
                        nsIDOMMozVoicemailStatus** aStatus)
{
  *aStatus = nullptr;

  // VoicemailStatusData exposes these as references, so we can simply assign
  // to them.
  VoicemailStatusData data;
  data.hasMessages() = aHasMessages;
  data.messageCount() = aMessageCount;
  data.returnNumber() = nsString(aReturnNumber);
  data.returnMessage() = nsString(aReturnMessage);

  nsCOMPtr<nsIDOMMozVoicemailStatus> status = new VoicemailStatus(data);
  status.swap(*aStatus);
  return NS_OK;
}

DOMCI_DATA(MozVoicemailStatus, VoicemailStatus)

NS_INTERFACE_MAP_BEGIN(VoicemailStatus)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMozVoicemailStatus)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(MozVoicemailStatus)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(VoicemailStatus)
NS_IMPL_RELEASE(VoicemailStatus)

const VoicemailStatusData&
VoicemailStatus::GetData() const
{
  LOG("XXX GetData");
  return mData;
}

NS_IMETHODIMP
VoicemailStatus::GetHasMessages(bool* aHasMessages) 
{
  *aHasMessages = mData.hasMessages(); //mHasMessages;
  return NS_OK;
}

NS_IMETHODIMP
VoicemailStatus::GetMessageCount(int32_t* aMessageCount)
{
  *aMessageCount = mData.messageCount();//mMessageCount;
  return NS_OK;
}

NS_IMETHODIMP
VoicemailStatus::GetReturnNumber(nsAString& aReturnNumber)
{
  aReturnNumber = mData.returnNumber(); //mReturnNumber;
  return NS_OK;
}

NS_IMETHODIMP
VoicemailStatus::GetReturnMessage(nsAString& aReturnMessage)
{
  aReturnMessage = mData.returnMessage();//mReturnMessage;
  return NS_OK;
}
