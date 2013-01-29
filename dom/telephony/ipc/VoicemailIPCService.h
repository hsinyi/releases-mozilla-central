/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_telephony_VoicemailIPCService_h
#define mozilla_dom_telephony_VoicemailIPCService_h

#include "nsIVoicemailService.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace dom {
namespace telephony {

class PVoicemailChild;

class VoicemailIPCService MOZ_FINAL : public nsIVoicemailService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIVOICEMAILSERVICE

private:
  static PVoicemailChild* GetVoicemailChild();
};

} // namespace telephony
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_telephony_VoicemailIPCService_h
