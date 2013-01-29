/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "VoicemailChild.h"
#include "mozilla/Services.h"
#include "mozilla/dom/ContentChild.h"

using namespace mozilla;
using namespace mozilla::dom::telephony;

namespace mozilla {
namespace dom {
namespace telephony {

VoicemailChild::VoicemailChild()
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

} // namespace telephony
} // namespace dom
} // namespace mozilla
