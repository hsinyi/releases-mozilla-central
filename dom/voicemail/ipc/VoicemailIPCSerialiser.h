/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_voicemail_VoicemailIPCSerialiser_h
#define mozilla_dom_voicemail_VoicemailIPCSerialiser_h

#include "ipc/IPCMessageUtils.h"
#include "nsIDOMMozVoicemail.h"
#include "mozilla/dom/voicemail/VoicemailStatus.h"

typedef nsIDOMMozVoicemailStatus* nsVoicemailStatus;

using namespace mozilla::dom;
using namespace mozilla::dom::voicemail;

namespace IPC {

/**
 * nsIDOMMozVoicemailStatus
 */
template <>
struct ParamTraits<nsIDOMMozVoicemailStatus*>
{
  typedef nsIDOMMozVoicemailStatus* paramType;

  // Function to serialize a VoicemailStatus
  static void Write(Message* aMsg, const paramType& aParam)
  {
    bool isNull = !aParam;
    WriteParam(aMsg, isNull);
    if (isNull) {
      return;
    }

    bool pBool;
    nsString pString;
    int32_t pLong;

    aParam->GetHasMessages(&pBool);
    WriteParam(aMsg, pBool);
    aParam->GetMessageCount(&pLong);
    WriteParam(aMsg, pLong);
    aParam->GetReturnNumber(pString);
    WriteParam(aMsg, pString);
    aParam->GetReturnMessage(pString);
    WriteParam(aMsg, pString);
  }

  // Function to de-serialize a VoicemailStatus
  static bool Read(const Message* aMsg, void **aIter, paramType* aResult)
  {
    bool isNull;
    if (!ReadParam(aMsg, aIter, &isNull)) {
        return false;
    }

    if (isNull) {
      *aResult = nullptr;
      return true;
    }

    bool hasMessages;
    int32_t messageCount;
    nsString returnNumber;
    nsString returnMessage;

    if (!(ReadParam(aMsg, aIter, &hasMessages) &&
          ReadParam(aMsg, aIter, &messageCount) &&
          ReadParam(aMsg, aIter, &returnNumber) &&
          ReadParam(aMsg, aIter, &returnMessage))) {
      return false;
    }

    *aResult = new VoicemailStatus(hasMessages,
                                   messageCount,
                                   returnNumber,
                                   returnMessage);

    return true; 

  }
};

} // namespace IPC

#endif
