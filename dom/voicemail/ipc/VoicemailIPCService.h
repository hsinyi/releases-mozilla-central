/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_voicemail_VoicemailIPCService_h
#define mozilla_dom_voicemail_VoicemailIPCService_h

#include "nsIVoicemailProvider.h"
#include "mozilla/Attributes.h"

#include "nsCOMPtr.h"
#include "nsTArray.h"

class PVoicemailChild;

namespace mozilla {
namespace dom {
namespace voicemail {

class VoicemailIPCService MOZ_FINAL : public nsIVoicemailProvider
                                    , public nsIVoicemailListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIVOICEMAILPROVIDER
  NS_DECL_NSIVOICEMAILLISTENER

  VoicemailIPCService ();

private:
  virtual ~VoicemailIPCService();

  void ReleaseIPCChild();

  PVoicemailChild* mChild;

  nsTArray< nsCOMPtr<nsIVoicemailListener> > mListeners;
};

} // namespace voicemail
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_voicemail_VoicemailIPCService_h
