/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=40: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_telephony_telephony_h__
#define mozilla_dom_telephony_telephony_h__

#include "TelephonyCommon.h"

#include "TelephonyManager.h"
#include "nsIDOMTelephony.h"
#include "nsIDOMTelephonyCall.h"
#include "nsIRadioInterfaceLayer.h"

class nsIScriptContext;
class nsPIDOMWindow;

BEGIN_TELEPHONY_NAMESPACE

class Telephony : public nsDOMEventTargetHelper,
                  public nsIDOMTelephony
{
 // TelephonyManager* mTelephonyManager;
  nsRefPtr<TelephonyManager> mTelephonyManager;

  nsCOMPtr<nsIRILContentHelper> mRIL;
  nsCOMPtr<nsIRILTelephonyCallback> mRILTelephonyCallback;

  TelephonyCall* mActiveCall;
  nsTArray<nsRefPtr<TelephonyCall> > mCalls;
  
  // Cached calls array object. Cleared whenever mCalls changes and then rebuilt
  // once a page looks for the liveCalls attribute.
  JSObject* mCallsArray;

  bool mRooted;
  uint32_t mPhoneIndex;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMTELEPHONY
  NS_DECL_NSIRILTELEPHONYCALLBACK
  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(
                                                   Telephony,
                                                   nsDOMEventTargetHelper)

  static already_AddRefed<Telephony>
  Create(TelephonyManager* aTelephonyManager, uint32_t aPhoneIndex, nsIRILContentHelper* aRIL);

  nsIDOMEventTarget*
  ToIDOMEventTarget() const
  {
    return static_cast<nsDOMEventTargetHelper*>(
             const_cast<Telephony*>(this));
  }

  nsISupports*
  ToISupports() const
  {
    return ToIDOMEventTarget();
  }

  void 
  AddCall(TelephonyCall* aCall);

  void
  RemoveCall(TelephonyCall* aCall);

  nsIRILContentHelper*
  RIL() const
  {
    return mRIL;
  }

  uint32_t PhoneIndex() const
  {
    return mPhoneIndex;
  }
private:
  Telephony();
  ~Telephony();

  already_AddRefed<TelephonyCall>
  CreateNewDialingCall(const nsAString& aNumber);

  nsresult
  NotifyCallsChanged(TelephonyCall* aCall);

  nsresult
  DialInternal(bool isEmergency,
               const nsAString& aNumber,
               nsIDOMTelephonyCall** aResult);

  class RILTelephonyCallback : public nsIRILTelephonyCallback
  {
    Telephony* mTelephony;

  public:
    NS_DECL_ISUPPORTS
    NS_FORWARD_NSIRILTELEPHONYCALLBACK(mTelephony->)

    RILTelephonyCallback(Telephony* aTelephony)
    : mTelephony(aTelephony)
    {
      NS_ASSERTION(mTelephony, "Null pointer!");
    }
  };
};

END_TELEPHONY_NAMESPACE

#endif // mozilla_dom_telephony_telephony_h__
