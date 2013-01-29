/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_telephony_VoicemailChild_h
#define mozilla_dom_telephony_VoicemailChild_h

#include "mozilla/dom/telephony/PVoicemailChild.h"

namespace mozilla {
namespace dom {
namespace telephony {

class VoicemailChild : public PVoicemailChild
{
public:
  VoicemailChild();

protected:
  virtual ~VoicemailChild();

  virtual void
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;
};

} // namespace telephony
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_telephony_VoicemailChild_h
