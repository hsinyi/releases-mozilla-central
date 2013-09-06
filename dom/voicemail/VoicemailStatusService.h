/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_voicemail_VoicemailStatusService_h
#define mozilla_dom_voicemail_VoicemailStatusService_h

#include "nsIVoicemailStatusService.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/StaticPtr.h"

namespace mozilla {
namespace dom {
namespace voicemail {

class VoicemailStatusService MOZ_FINAL : public nsIVoicemailStatusService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIVOICEMAILSTATUSSERVICE

  static already_AddRefed<VoicemailStatusService> GetInstance();

private:
  static StaticRefPtr<VoicemailStatusService> sSingleton;

};

} // namespace voicemail
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_voicemail_VoicemailStatusService_h
