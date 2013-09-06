/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_voicemail_VoicemailStatus_h
#define mozilla_dom_voicemail_VoicemailStatus_h

#include "mozilla/dom/voicemail/VoicemailTypes.h"
#include "nsIDOMMozVoicemailStatus.h"
#include "nsString.h"

namespace mozilla {
namespace dom {
namespace voicemail {

class VoicemailStatus : public nsIDOMMozVoicemailStatus
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMMOZVOICEMAILSTATUS

  VoicemailStatus(bool aHasMessages,
                  int32_t aMessageCount,
                  const nsString& aReturnNumber,
                  const nsString& aReturnMessage);
  VoicemailStatus(const VoicemailStatusData& aData);

  static nsresult Create(bool aHasMessages,
                         int32_t aMessageCount,
                         const nsAString& aReturnNumber,
                         const nsAString& aReturnMessage,
                         JSContext* aCx,
                         nsIDOMMozVoicemailStatus** aStatus);
  const VoicemailStatusData& GetData() const;
private:
  bool mHasMessages;
  int32_t mMessageCount;
  nsString mReturnNumber;
  nsString mReturnMessage;

  VoicemailStatusData mData;
};

} // namespace voicemail
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_voicemail_VoicemailStatus_h 
