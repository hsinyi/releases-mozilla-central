/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_voicemail_VoicemailParent_h
#define mozilla_dom_voicemail_VoicemailParent_h

#include "mozilla/dom/PContentParent.h"
#include "mozilla/dom/voicemail/PVoicemailParent.h"
#include "nsIVoicemailProvider.h"

namespace mozilla {
namespace dom {

class ContentParent;

} // namespace dom
} // namespace mozilla

namespace mozilla {
namespace dom {
namespace voicemail {

class VoicemailParent MOZ_FINAL : public PVoicemailParent
                                , public nsIVoicemailListener
{
  friend class mozilla::dom::ContentParent;
  bool mActorDestroyed;
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIVOICEMAILLISTENER

  VoicemailParent () : mActorDestroyed (false) {}
protected:
  virtual ~VoicemailParent() {}

  virtual void
  ActorDestroy(ActorDestroyReason why);

  bool
  RegisterListener();

  virtual bool
  RecvGetNumber(nsString* aNumber) MOZ_OVERRIDE;

  virtual bool
  RecvGetDisplayName(nsString* aDisplayName) MOZ_OVERRIDE;

  bool
  GetVoicemailStatusDataFromStatus(nsIDOMMozVoicemailStatus* aStatus,
                                   VoicemailStatusData& aData) MOZ_OVERRIDE;
};

} // namespace voicemail
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_voicemail_VoicemailParent_h
