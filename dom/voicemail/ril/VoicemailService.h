/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_voicemail_VoicemailService_h
#define mozilla_dom_voicemail_VoicemailService_h

#include "nsIVoicemailProvider.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsIRadioInterfaceLayer.h"

namespace mozilla {
namespace dom {
namespace voicemail {

class VoicemailService : public nsIVoicemailProvider
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIVOICEMAILPROVIDER

  VoicemailService ();

private:
  virtual ~VoicemailService();

  nsCOMPtr<nsIRadioInterfaceLayer> mRIL;
};

} // namespace voicemail
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_voicemail_VoicemailService_h
