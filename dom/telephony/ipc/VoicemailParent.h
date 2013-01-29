/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_telephony_VoicemailParent_h
#define mozilla_dom_telephony_VoicemailParent_h

#include "mozilla/dom/telephony/PVoicemailParent.h"
#include "mozilla/dom/PBlob.h"

namespace mozilla {
namespace dom {

class ContentParent;

} // namespace dom
} // namespace mozilla

namespace mozilla {
namespace dom {
namespace telephony {

class VoicemailParent : public PVoicemailParent
{
  friend class mozilla::dom::ContentParent;

public:

protected:
  virtual bool
  RecvTestRegister(const int32_t& aId, int32_t* aReturnId) MOZ_OVERRIDE;

  VoicemailParent();
  virtual ~VoicemailParent();

  virtual void
  ActorDestroy(ActorDestroyReason why);
};

} // namespace telephony
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_telephony_VoicemailParent_h
