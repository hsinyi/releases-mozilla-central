/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_voicemail_VoicemailServiceFactory_h
#define mozilla_dom_voicemail_VoicemailServiceFactory_h

#include "nsCOMPtr.h"

class nsIVoicemailProvider;

namespace mozilla {
namespace dom {
namespace voicemail {

class VoicemailServiceFactory
{
public:
  static already_AddRefed<nsIVoicemailProvider> CreateVoicemailService();
};

} // namespace voicemail
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_voicemail_VoicemailServiceFactory_h
