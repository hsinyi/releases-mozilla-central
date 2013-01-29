/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_telephony_VoicemailService_h
#define mozilla_dom_telephony_VoicemailService_h

#include "nsIVoicemailService.h"
#include "nsCOMPtr.h"
#include "nsIRadioInterfaceLayer.h"

namespace mozilla {
namespace dom {
namespace telephony {

class VoicemailService : public nsIVoicemailService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIVOICEMAILSERVICE
  VoicemailService();

protected:
  nsCOMPtr<nsIRadioInterfaceLayer> mRIL;
};

} // namespace telephony
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_telephony_VoicemailService_h
