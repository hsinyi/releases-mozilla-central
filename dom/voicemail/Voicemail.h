/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=40: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_voicemail_Voicemail_h__
#define mozilla_dom_voicemail_Voicemail_h__

#include "nsDOMEvent.h"
#include "nsDOMEventTargetHelper.h"
#include "nsIDOMMozVoicemail.h"
#include "nsIVoicemailProvider.h"
#include "mozilla/Attributes.h"

class nsPIDOMWindow;
class nsIDOMMozVoicemailStatus;

namespace mozilla {
namespace dom {

class Voicemail : public nsDOMEventTargetHelper,
                  public nsIDOMMozVoicemail
{
  class Listener;
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMMOZVOICEMAIL
  NS_DECL_NSIVOICEMAILLISTENER

  NS_REALLY_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper)

  Voicemail(nsPIDOMWindow* aWindow, nsIVoicemailProvider* aProvider);
  virtual ~Voicemail();

private:
  nsCOMPtr<nsIVoicemailProvider> mProvider;
  nsRefPtr<Listener> mListener;
};

} // namespace dom
} // namespace mozilla

nsresult
NS_NewVoicemail(nsPIDOMWindow* aWindow, nsIDOMMozVoicemail** aVoicemail);

#endif // mozilla_dom_voicemail_voicemail_h__
