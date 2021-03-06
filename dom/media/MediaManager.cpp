/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "MediaManager.h"

#include "MediaStreamGraph.h"
#include "nsIDOMFile.h"
#include "nsIEventTarget.h"
#include "nsIUUIDGenerator.h"
#include "nsIScriptGlobalObject.h"
#include "nsIPopupWindowManager.h"

// For PR_snprintf
#include "prprf.h"

#include "nsJSUtils.h"
#include "nsDOMFile.h"
#include "nsGlobalWindow.h"

#include "mozilla/Preferences.h"

/* Using WebRTC backend on Desktops (Mac, Windows, Linux), otherwise default */
#include "MediaEngineDefault.h"
#if defined(MOZ_WEBRTC)
#include "MediaEngineWebRTC.h"
#endif

namespace mozilla {

#ifdef PR_LOGGING
PRLogModuleInfo* gMediaManagerLog = PR_NewLogModule("MediaManager");
#define LOG(msg) PR_LOG(gMediaManagerLog, PR_LOG_DEBUG, msg)
#else
#define LOG(msg)
#endif


/**
 * Send an error back to content. The error is the form a string.
 * Do this only on the main thread. The success callback is also passed here
 * so it can be released correctly.
 */
class ErrorCallbackRunnable : public nsRunnable
{
public:
  ErrorCallbackRunnable(
    already_AddRefed<nsIDOMGetUserMediaSuccessCallback> aSuccess,
    already_AddRefed<nsIDOMGetUserMediaErrorCallback> aError,
    const nsString& aErrorMsg, uint64_t aWindowID)
    : mSuccess(aSuccess)
    , mError(aError)
    , mErrorMsg(aErrorMsg)
    , mWindowID(aWindowID) {}

  NS_IMETHOD
  Run()
  {
    // Only run if the window is still active.
    NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

    nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> success(mSuccess);
    nsCOMPtr<nsIDOMGetUserMediaErrorCallback> error(mError);

    {
      MutexAutoLock lock(MediaManager::Get()->GetMutex());
      WindowTable* activeWindows = MediaManager::Get()->GetActiveWindows();
      if (activeWindows->Get(mWindowID)) {
        error->OnError(mErrorMsg);
      }
    }
    return NS_OK;
  }

private:
  already_AddRefed<nsIDOMGetUserMediaSuccessCallback> mSuccess;
  already_AddRefed<nsIDOMGetUserMediaErrorCallback> mError;
  const nsString mErrorMsg;
  uint64_t mWindowID;
};

/**
 * Invoke the "onSuccess" callback in content. The callback will take a
 * DOMBlob in the case of {picture:true}, and a MediaStream in the case of
 * {audio:true} or {video:true}. There is a constructor available for each
 * form. Do this only on the main thread.
 */
class SuccessCallbackRunnable : public nsRunnable
{
public:
  SuccessCallbackRunnable(
    already_AddRefed<nsIDOMGetUserMediaSuccessCallback> aSuccess,
    already_AddRefed<nsIDOMGetUserMediaErrorCallback> aError,
    nsIDOMFile* aFile, uint64_t aWindowID)
    : mSuccess(aSuccess)
    , mError(aError)
    , mFile(aFile)
    , mWindowID(aWindowID) {}

  NS_IMETHOD
  Run()
  {
    // Only run if the window is still active.
    NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

    nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> success(mSuccess);
    nsCOMPtr<nsIDOMGetUserMediaErrorCallback> error(mError);

    {
      MutexAutoLock lock(MediaManager::Get()->GetMutex());
      WindowTable* activeWindows = MediaManager::Get()->GetActiveWindows();
      if (activeWindows->Get(mWindowID)) {
        // XPConnect is a magical unicorn.
        success->OnSuccess(mFile);
      }
    }
    return NS_OK;
  }

private:
  already_AddRefed<nsIDOMGetUserMediaSuccessCallback> mSuccess;
  already_AddRefed<nsIDOMGetUserMediaErrorCallback> mError;
  nsCOMPtr<nsIDOMFile> mFile;
  uint64_t mWindowID;
};

/**
 * Invoke the GetUserMediaDevices success callback. Wrapped in a runnable
 * so that it may be called on the main thread. The error callback is also
 * passed so it can be released correctly.
 */
class DeviceSuccessCallbackRunnable: public nsRunnable
{
public:
  DeviceSuccessCallbackRunnable(
    already_AddRefed<nsIGetUserMediaDevicesSuccessCallback> aSuccess,
    already_AddRefed<nsIDOMGetUserMediaErrorCallback> aError,
    const nsTArray<nsCOMPtr<nsIMediaDevice> >& aDevices)
    : mSuccess(aSuccess)
    , mError(aError)
    , mDevices(aDevices) {}

  NS_IMETHOD
  Run()
  {
    NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

    nsCOMPtr<nsIGetUserMediaDevicesSuccessCallback> success(mSuccess);
    nsCOMPtr<nsIDOMGetUserMediaErrorCallback> error(mError);

    nsCOMPtr<nsIWritableVariant> devices =
      do_CreateInstance("@mozilla.org/variant;1");

    int32_t len = mDevices.Length();
    if (len == 0) {
      devices->SetAsEmptyArray();
      success->OnSuccess(devices);
      return NS_OK;
    }

    nsTArray<nsIMediaDevice*> tmp(len);
    for (int32_t i = 0; i < len; i++) {
      tmp.AppendElement(mDevices.ElementAt(i));
    }

    devices->SetAsArray(nsIDataType::VTYPE_INTERFACE,
                        &NS_GET_IID(nsIMediaDevice),
                        mDevices.Length(),
                        const_cast<void*>(
                          static_cast<const void*>(tmp.Elements())
                        ));

    success->OnSuccess(devices);
    return NS_OK;
  }

private:
  already_AddRefed<nsIGetUserMediaDevicesSuccessCallback> mSuccess;
  already_AddRefed<nsIDOMGetUserMediaErrorCallback> mError;
  nsTArray<nsCOMPtr<nsIMediaDevice> > mDevices;
};

/**
 * nsIMediaDevice implementation.
 */
NS_IMPL_THREADSAFE_ISUPPORTS1(MediaDevice, nsIMediaDevice)

NS_IMETHODIMP
MediaDevice::GetName(nsAString& aName)
{
  aName.Assign(mName);
  return NS_OK;
}

NS_IMETHODIMP
MediaDevice::GetType(nsAString& aType)
{
  aType.Assign(mType);
  return NS_OK;
}

MediaEngineSource*
MediaDevice::GetSource()
{
  return mSource;
}

/**
 * Creates a MediaStream, attaches a listener and fires off a success callback
 * to the DOM with the stream. We also pass in the error callback so it can
 * be released correctly.
 *
 * All of this must be done on the main thread!
 *
 * Note that the various GetUserMedia Runnable classes currently allow for
 * two streams.  If we ever need to support getting more than two streams
 * at once, we could convert everything to nsTArray<nsRefPtr<blah> >'s,
 * though that would complicate the constructors some.  Currently the
 * GetUserMedia spec does not allow for more than 2 streams to be obtained in
 * one call, to simplify handling of constraints.
 */
class GetUserMediaStreamRunnable : public nsRunnable
{
public:
  GetUserMediaStreamRunnable(
    already_AddRefed<nsIDOMGetUserMediaSuccessCallback> aSuccess,
    already_AddRefed<nsIDOMGetUserMediaErrorCallback> aError,
    StreamListeners* aListeners,
    uint64_t aWindowID,
    MediaEngineSource* aAudioSource,
    MediaEngineSource* aVideoSource)
    : mSuccess(aSuccess)
    , mError(aError)
    , mAudioSource(aAudioSource)
    , mVideoSource(aVideoSource)
    , mListeners(aListeners)
    , mWindowID(aWindowID) {}

  ~GetUserMediaStreamRunnable() {}

  NS_IMETHOD
  Run()
  {
    NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

    // Create a media stream.
    nsRefPtr<nsDOMLocalMediaStream> stream;
    uint32_t hints = (mAudioSource ? nsDOMMediaStream::HINT_CONTENTS_AUDIO : 0);
    hints |= (mVideoSource ? nsDOMMediaStream::HINT_CONTENTS_VIDEO : 0);

    stream = nsDOMLocalMediaStream::CreateInputStream(hints);

    nsPIDOMWindow *window = static_cast<nsPIDOMWindow*>
      (nsGlobalWindow::GetInnerWindowWithId(mWindowID));
    WindowTable* activeWindows = MediaManager::Get()->GetActiveWindows();
    {
      MutexAutoLock lock(MediaManager::Get()->GetMutex());

      if (!stream) {
        if (activeWindows->Get(mWindowID)) {
          nsCOMPtr<nsIDOMGetUserMediaErrorCallback> error(mError);
          LOG(("Returning error for getUserMedia() - no stream"));
          error->OnError(NS_LITERAL_STRING("NO_STREAM"));
        }
        return NS_OK;
      }
    }
    if (window && window->GetExtantDoc()) {
      stream->CombineWithPrincipal(window->GetExtantDoc()->NodePrincipal());
    }

    // Ensure there's a thread for gum to proxy to off main thread
    nsIThread *mediaThread = MediaManager::GetThread();

    // Add our listener. We'll call Start() on the source when get a callback
    // that the MediaStream has started consuming. The listener is freed
    // when the page is invalidated (on navigation or close).
    GetUserMediaCallbackMediaStreamListener* listener =
      new GetUserMediaCallbackMediaStreamListener(mediaThread, stream,
                                                  mAudioSource,
                                                  mVideoSource);
    stream->GetStream()->AddListener(listener);

    // No need for locking because we always do this in the main thread.
    mListeners->AppendElement(listener);

    // Dispatch to the media thread to ask it to start the sources,
    // because that can take a while
    nsRefPtr<MediaOperationRunnable> runnable(
      new MediaOperationRunnable(MEDIA_START, stream,
                                 mAudioSource, mVideoSource));
    mediaThread->Dispatch(runnable, NS_DISPATCH_NORMAL);

    // We're in the main thread, so no worries here either.
    nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> success(mSuccess);
    nsCOMPtr<nsIDOMGetUserMediaErrorCallback> error(mError);

    {
      MutexAutoLock lock(MediaManager::Get()->GetMutex());
      if (activeWindows->Get(mWindowID)) {
        LOG(("Returning success for getUserMedia()"));
        success->OnSuccess(static_cast<nsIDOMLocalMediaStream*>(stream));
      }
    }

    return NS_OK;
  }

private:
  already_AddRefed<nsIDOMGetUserMediaSuccessCallback> mSuccess;
  already_AddRefed<nsIDOMGetUserMediaErrorCallback> mError;
  nsRefPtr<MediaEngineSource> mAudioSource;
  nsRefPtr<MediaEngineSource> mVideoSource;
  StreamListeners* mListeners;
  uint64_t mWindowID;
};

/**
 * Runs on a seperate thread and is responsible for enumerating devices.
 * Depending on whether a picture or stream was asked for, either
 * ProcessGetUserMedia or ProcessGetUserMediaSnapshot is called, and the results
 * are sent back to the DOM.
 *
 * Do not run this on the main thread. The success and error callbacks *MUST*
 * be dispatched on the main thread!
 */
class GetUserMediaRunnable : public nsRunnable
{
public:
  /**
   * The caller can choose to provide a MediaDevice as the last argument,
   * if one is not provided, a default device is automatically chosen.
   */
  GetUserMediaRunnable(bool aAudio, bool aVideo, bool aPicture,
    already_AddRefed<nsIDOMGetUserMediaSuccessCallback> aSuccess,
    already_AddRefed<nsIDOMGetUserMediaErrorCallback> aError,
    StreamListeners* aListeners, uint64_t aWindowID,
    MediaDevice* aAudioDevice, MediaDevice* aVideoDevice)
    : mAudio(aAudio)
    , mVideo(aVideo)
    , mPicture(aPicture)
    , mSuccess(aSuccess)
    , mError(aError)
    , mListeners(aListeners)
    , mWindowID(aWindowID)
    , mDeviceChosen(true)
    , mBackendChosen(false)
    {
      if (mAudio) {
        mAudioDevice = aAudioDevice;
      }
      if (mVideo) {
        mVideoDevice = aVideoDevice;
      }
    }

  GetUserMediaRunnable(bool aAudio, bool aVideo, bool aPicture,
    already_AddRefed<nsIDOMGetUserMediaSuccessCallback> aSuccess,
    already_AddRefed<nsIDOMGetUserMediaErrorCallback> aError,
    StreamListeners* aListeners, uint64_t aWindowID)
    : mAudio(aAudio)
    , mVideo(aVideo)
    , mPicture(aPicture)
    , mSuccess(aSuccess)
    , mError(aError)
    , mListeners(aListeners)
    , mWindowID(aWindowID)
    , mDeviceChosen(false)
    , mBackendChosen(false) {}

  /**
   * The caller can also choose to provide their own backend instead of
   * using the one provided by MediaManager::GetBackend.
   */
  GetUserMediaRunnable(bool aAudio, bool aVideo,
    already_AddRefed<nsIDOMGetUserMediaSuccessCallback> aSuccess,
    already_AddRefed<nsIDOMGetUserMediaErrorCallback> aError,
    StreamListeners* aListeners, uint64_t aWindowID, MediaEngine* aBackend)
    : mAudio(aAudio)
    , mVideo(aVideo)
    , mPicture(false)
    , mSuccess(aSuccess)
    , mError(aError)
    , mListeners(aListeners)
    , mWindowID(aWindowID)
    , mDeviceChosen(false)
    , mBackendChosen(true)
    , mBackend(aBackend) {}

  ~GetUserMediaRunnable() {
    if (mBackendChosen) {
      delete mBackend;
    }
  }

  NS_IMETHOD
  Run()
  {
    NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");

    mManager = MediaManager::Get();

    // Was a backend provided?
    if (!mBackendChosen) {
      mBackend = mManager->GetBackend();
    }

    // Was a device provided?
    if (!mDeviceChosen) {
      nsresult rv = SelectDevice();
      if (rv != NS_OK) {
        return rv;
      }
    }

    // It is an error if audio or video are requested along with picture.
    if (mPicture && (mAudio || mVideo)) {
      NS_DispatchToMainThread(new ErrorCallbackRunnable(
        mSuccess, mError, NS_LITERAL_STRING("NOT_SUPPORTED_ERR"), mWindowID
      ));
      return NS_OK;
    }

    if (mPicture) {
      ProcessGetUserMediaSnapshot(mVideoDevice->GetSource(), 0);
      return NS_OK;
    }

    ProcessGetUserMedia(mAudio ? mAudioDevice->GetSource() : nullptr,
                        mVideo ? mVideoDevice->GetSource() : nullptr);
    return NS_OK;
  }

  nsresult
  Denied()
  {
    if (NS_IsMainThread()) {
      nsCOMPtr<nsIDOMGetUserMediaErrorCallback> error(mError);
      error->OnError(NS_LITERAL_STRING("PERMISSION_DENIED"));
    } else {
      NS_DispatchToMainThread(new ErrorCallbackRunnable(
        mSuccess, mError, NS_LITERAL_STRING("PERMISSION_DENIED"), mWindowID
      ));
    }

    return NS_OK;
  }

  nsresult
  SetAudioDevice(MediaDevice* aAudioDevice)
  {
    mAudioDevice = aAudioDevice;
    mDeviceChosen = true;
    return NS_OK;
  }

  nsresult
  SetVideoDevice(MediaDevice* aVideoDevice)
  {
    mVideoDevice = aVideoDevice;
    mDeviceChosen = true;
    return NS_OK;
  }

  nsresult
  SelectDevice()
  {
    bool found = false;
    uint32_t count;
    if (mPicture || mVideo) {
      nsTArray<nsRefPtr<MediaEngineVideoSource> > videoSources;
      mBackend->EnumerateVideoDevices(&videoSources);

      count = videoSources.Length();
      if (count <= 0) {
        NS_DispatchToMainThread(new ErrorCallbackRunnable(
          mSuccess, mError, NS_LITERAL_STRING("NO_DEVICES_FOUND"), mWindowID
        ));
        return NS_ERROR_FAILURE;
      }

      // Pick the first available device.
      for (uint32_t i = 0; i < count; i++) {
        nsRefPtr<MediaEngineVideoSource> vSource = videoSources[i];
        if (vSource->IsAvailable()) {
          found = true;
          mVideoDevice = new MediaDevice(videoSources[i]);
        }
      }

      if (!found) {
        NS_DispatchToMainThread(new ErrorCallbackRunnable(
          mSuccess, mError, NS_LITERAL_STRING("HARDWARE_UNAVAILABLE"), mWindowID
        ));
        return NS_ERROR_FAILURE;
      }
      LOG(("Selected video device"));
    }
    if (mAudio) {
      nsTArray<nsRefPtr<MediaEngineAudioSource> > audioSources;
      mBackend->EnumerateAudioDevices(&audioSources);

      count = audioSources.Length();
      if (count <= 0) {
        NS_DispatchToMainThread(new ErrorCallbackRunnable(
          mSuccess, mError, NS_LITERAL_STRING("NO_DEVICES_FOUND"), mWindowID
        ));
        return NS_ERROR_FAILURE;
      }

      for (uint32_t i = 0; i < count; i++) {
        nsRefPtr<MediaEngineAudioSource> aSource = audioSources[i];
        if (aSource->IsAvailable()) {
          found = true;
          mAudioDevice = new MediaDevice(audioSources[i]);
        }
      }

      if (!found) {
        NS_DispatchToMainThread(new ErrorCallbackRunnable(
          mSuccess, mError, NS_LITERAL_STRING("HARDWARE_UNAVAILABLE"), mWindowID
        ));
        return NS_ERROR_FAILURE;
      }
      LOG(("Selected audio device"));
    }

    return NS_OK;
  }

  /**
   * Allocates a video or audio device and returns a MediaStream via
   * a GetUserMediaStreamRunnable. Runs off the main thread.
   */
  void
  ProcessGetUserMedia(MediaEngineSource* aAudioSource, MediaEngineSource* aVideoSource)
  {
    nsresult rv;
    if (aAudioSource) {
      rv = aAudioSource->Allocate();
      if (NS_FAILED(rv)) {
        LOG(("Failed to allocate audiosource %d",rv));
        NS_DispatchToMainThread(new ErrorCallbackRunnable(
                                  mSuccess, mError, NS_LITERAL_STRING("HARDWARE_UNAVAILABLE"), mWindowID
                                                          ));
        return;
      }
    }
    if (aVideoSource) {
      rv = aVideoSource->Allocate();
      if (NS_FAILED(rv)) {
        LOG(("Failed to allocate videosource %d\n",rv));
        if (aAudioSource) {
          aAudioSource->Deallocate();
        }
        NS_DispatchToMainThread(new ErrorCallbackRunnable(
          mSuccess, mError, NS_LITERAL_STRING("HARDWARE_UNAVAILABLE"), mWindowID
                                                          ));
        return;
      }
    }

    NS_DispatchToMainThread(new GetUserMediaStreamRunnable(
      mSuccess, mError, mListeners, mWindowID, aAudioSource, aVideoSource
    ));
    return;
  }

  /**
   * Allocates a video device, takes a snapshot and returns a DOMFile via
   * a SuccessRunnable or an error via the ErrorRunnable. Off the main thread.
   */
  void
  ProcessGetUserMediaSnapshot(MediaEngineSource* aSource, int aDuration)
  {
    nsresult rv = aSource->Allocate();
    if (NS_FAILED(rv)) {
      NS_DispatchToMainThread(new ErrorCallbackRunnable(
        mSuccess, mError, NS_LITERAL_STRING("HARDWARE_UNAVAILABLE"), mWindowID
      ));
      return;
    }

    /**
     * Display picture capture UI here before calling Snapshot() - Bug 748835.
     */
    nsCOMPtr<nsIDOMFile> file;
    aSource->Snapshot(aDuration, getter_AddRefs(file));
    aSource->Deallocate();

    NS_DispatchToMainThread(new SuccessCallbackRunnable(
      mSuccess, mError, file, mWindowID
    ));
    return;
  }

private:
  bool mAudio;
  bool mVideo;
  bool mPicture;

  already_AddRefed<nsIDOMGetUserMediaSuccessCallback> mSuccess;
  already_AddRefed<nsIDOMGetUserMediaErrorCallback> mError;
  StreamListeners* mListeners;
  uint64_t mWindowID;
  nsRefPtr<MediaDevice> mAudioDevice;
  nsRefPtr<MediaDevice> mVideoDevice;

  bool mDeviceChosen;
  bool mBackendChosen;

  MediaEngine* mBackend;
  MediaManager* mManager;
};

/**
 * Similar to GetUserMediaRunnable, but used for the chrome-only
 * GetUserMediaDevices function. Enumerates a list of audio & video devices,
 * wraps them up in nsIMediaDevice objects and returns it to the success
 * callback.
 */
class GetUserMediaDevicesRunnable : public nsRunnable
{
public:
  GetUserMediaDevicesRunnable(
    already_AddRefed<nsIGetUserMediaDevicesSuccessCallback> aSuccess,
    already_AddRefed<nsIDOMGetUserMediaErrorCallback> aError)
    : mSuccess(aSuccess)
    , mError(aError) {}
  ~GetUserMediaDevicesRunnable() {}

  NS_IMETHOD
  Run()
  {
    NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");

    uint32_t audioCount, videoCount, i;
    MediaManager* manager = MediaManager::Get();

    nsTArray<nsRefPtr<MediaEngineVideoSource> > videoSources;
    manager->GetBackend()->EnumerateVideoDevices(&videoSources);
    videoCount = videoSources.Length();

    nsTArray<nsRefPtr<MediaEngineAudioSource> > audioSources;
    manager->GetBackend()->EnumerateAudioDevices(&audioSources);
    audioCount = audioSources.Length();

    nsTArray<nsCOMPtr<nsIMediaDevice> > *devices =
      new nsTArray<nsCOMPtr<nsIMediaDevice> >;

    /**
     * We only display available devices in the UI for now. We can easily
     * change this later, when we implement a more sophisticated UI that
     * lets the user revoke a device currently held by another tab (or
     * we decide to provide a stream from a device already allocated).
     */
    for (i = 0; i < videoCount; i++) {
      nsRefPtr<MediaEngineVideoSource> vSource = videoSources[i];
      if (vSource->IsAvailable()) {
        devices->AppendElement(new MediaDevice(vSource));
      }
    }
    for (i = 0; i < audioCount; i++) {
      nsRefPtr<MediaEngineAudioSource> aSource = audioSources[i];
      if (aSource->IsAvailable()) {
        devices->AppendElement(new MediaDevice(aSource));
      }
    }

    NS_DispatchToMainThread(new DeviceSuccessCallbackRunnable(
      mSuccess, mError, *devices
    ));
    return NS_OK;
  }

private:
  already_AddRefed<nsIGetUserMediaDevicesSuccessCallback> mSuccess;
  already_AddRefed<nsIDOMGetUserMediaErrorCallback> mError;
};

nsRefPtr<MediaManager> MediaManager::sSingleton;

NS_IMPL_THREADSAFE_ISUPPORTS1(MediaManager, nsIObserver)

/**
 * The entry point for this file. A call from Navigator::mozGetUserMedia
 * will end up here. MediaManager is a singleton that is responsible
 * for handling all incoming getUserMedia calls from every window.
 */
nsresult
MediaManager::GetUserMedia(bool aPrivileged, nsPIDOMWindow* aWindow,
  nsIMediaStreamOptions* aParams,
  nsIDOMGetUserMediaSuccessCallback* aOnSuccess,
  nsIDOMGetUserMediaErrorCallback* aOnError)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  NS_ENSURE_TRUE(aParams, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(aWindow, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(aOnError, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(aOnSuccess, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> onSuccess(aOnSuccess);
  nsCOMPtr<nsIDOMGetUserMediaErrorCallback> onError(aOnError);

  /* Get options */
  nsresult rv;
  bool fake, audio, video, picture;

  rv = aParams->GetFake(&fake);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aParams->GetPicture(&picture);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aParams->GetAudio(&audio);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aParams->GetVideo(&video);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMediaDevice> audiodevice;
  rv = aParams->GetAudioDevice(getter_AddRefs(audiodevice));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMediaDevice> videodevice;
  rv = aParams->GetVideoDevice(getter_AddRefs(videodevice));
  NS_ENSURE_SUCCESS(rv, rv);

  // If a device was provided, make sure it support the type of stream requested.
  // Doesn't handle hard-specifying both audio and video
  if (audiodevice) {
    nsString type;
    audiodevice->GetType(type);
    if (audio && !type.EqualsLiteral("audio")) {
      return NS_ERROR_FAILURE;
    }
  }
  if (videodevice) {
    nsString type;
    videodevice->GetType(type);
    if ((picture || video) && !type.EqualsLiteral("video")) {
        return NS_ERROR_FAILURE;
    }
  }

  // We only support "front" or "back". TBD: Send to GetUserMediaRunnable.
  nsString cameraType;
  rv = aParams->GetCamera(cameraType);
  NS_ENSURE_SUCCESS(rv, rv);

  /**
   * If we were asked to get a picture, before getting a snapshot, we check if
   * the calling page is allowed to open a popup. We do this because
   * {picture:true} will open a new "window" to let the user preview or select
   * an image, on Android. The desktop UI for {picture:true} is TBD, at which
   * may point we can decide whether to extend this test there as well.
   */
#if !defined(MOZ_WEBRTC)
  if (picture && !aPrivileged) {
    if (aWindow->GetPopupControlState() > openControlled) {
      nsCOMPtr<nsIPopupWindowManager> pm =
        do_GetService(NS_POPUPWINDOWMANAGER_CONTRACTID);
      if (!pm) {
        return NS_OK;
      }
      uint32_t permission;
      nsCOMPtr<nsIDocument> doc = aWindow->GetExtantDoc();
      pm->TestPermission(doc->NodePrincipal(), &permission);
      if ((permission == nsIPopupWindowManager::DENY_POPUP)) {
        nsCOMPtr<nsIDOMDocument> domDoc = aWindow->GetExtantDocument();
        nsGlobalWindow::FirePopupBlockedEvent(
          domDoc, aWindow, nullptr, EmptyString(), EmptyString()
        );
        return NS_OK;
      }
    }
  }
#endif

  static bool created = false;
  if (!created) {
    // Force MediaManager to startup before we try to access it from other threads
    // Hack: should init singleton earlier unless it's expensive (mem or CPU)
    (void) MediaManager::Get();
  }

  // Store the WindowID in a hash table and mark as active. The entry is removed
  // when this window is closed or navigated away from.
  uint64_t windowID = aWindow->WindowID();
  nsRefPtr<GetUserMediaRunnable> gUMRunnable;
  {
    MutexAutoLock lock(mMutex);
    StreamListeners* listeners = mActiveWindows.Get(windowID);
    if (!listeners) {
      listeners = new StreamListeners;
      mActiveWindows.Put(windowID, listeners);
    }

    // Developer preference for turning off permission check.
    if (Preferences::GetBool("media.navigator.permission.disabled", false)) {
      aPrivileged = true;
    }

    /**
     * Pass runnables along to GetUserMediaRunnable so it can add the
     * MediaStreamListener to the runnable list. The last argument can
     * optionally be a MediaDevice object, which should provided if one was
     * selected by the user via the UI, or was provided by privileged code
     * via the device: attribute via nsIMediaStreamOptions.
     *
     * If a fake stream was requested, we force the use of the default backend.
     */
    if (fake) {
      // Fake stream from default backend.
      gUMRunnable = new GetUserMediaRunnable(
        audio, video, onSuccess.forget(), onError.forget(), listeners,
        windowID, new MediaEngineDefault()
                                             );
    } else if (audiodevice || videodevice) {
      // Stream from provided device.
      gUMRunnable = new GetUserMediaRunnable(
        audio, video, picture, onSuccess.forget(), onError.forget(), listeners,
        windowID,
        static_cast<MediaDevice*>(audiodevice.get()),
        static_cast<MediaDevice*>(videodevice.get())
                                             );
    } else {
      // Stream from default device from WebRTC backend.
      gUMRunnable = new GetUserMediaRunnable(
        audio, video, picture, onSuccess.forget(), onError.forget(), listeners,
        windowID
                                             );
    }
  }


#ifdef ANDROID
  if (picture) {
    // ShowFilePickerForMimeType() must run on the Main Thread! (on Android)
    NS_DispatchToMainThread(gUMRunnable);
  }
  // XXX No support for Audio or Video in Android yet
#else
  // XXX No full support for picture in Desktop yet (needs proper UI)
  if (aPrivileged || fake) {
    (void) MediaManager::GetThread();
    mMediaThread->Dispatch(gUMRunnable, NS_DISPATCH_NORMAL);
  } else {
    // Ask for user permission, and dispatch runnable (or not) when a response
    // is received via an observer notification. Each call is paired with its
    // runnable by a GUID.
    nsresult rv;
    nsCOMPtr<nsIUUIDGenerator> uuidgen =
      do_GetService("@mozilla.org/uuid-generator;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // Generate a call ID.
    nsID id;
    rv = uuidgen->GenerateUUIDInPlace(&id);
    NS_ENSURE_SUCCESS(rv, rv);

    char buffer[NSID_LENGTH];
    id.ToProvidedString(buffer);
    NS_ConvertUTF8toUTF16 callID(buffer);

    // Store the current callback.
    {
      MutexAutoLock lock(mMutex);
      mActiveCallbacks.Put(callID, gUMRunnable);
    }

    // Construct JSON structure with both the windowID and the callID.
    nsAutoString data;
    data.Append(NS_LITERAL_STRING("{\"windowID\":"));

    // Convert window ID to string.
    char windowBuffer[32];
    PR_snprintf(windowBuffer, 32, "%llu", aWindow->GetOuterWindow()->WindowID());
    data.Append(NS_ConvertUTF8toUTF16(windowBuffer));

    data.Append(NS_LITERAL_STRING(", \"callID\":\""));
    data.Append(callID);
    data.Append(NS_LITERAL_STRING("\"}"));

    nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
    obs->NotifyObservers(aParams, "getUserMedia:request", data.get());
  }
#endif

  return NS_OK;
}

nsresult
MediaManager::GetUserMediaDevices(nsPIDOMWindow* aWindow,
  nsIGetUserMediaDevicesSuccessCallback* aOnSuccess,
  nsIDOMGetUserMediaErrorCallback* aOnError)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  NS_ENSURE_TRUE(aOnError, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(aOnSuccess, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIGetUserMediaDevicesSuccessCallback> onSuccess(aOnSuccess);
  nsCOMPtr<nsIDOMGetUserMediaErrorCallback> onError(aOnError);

  nsCOMPtr<nsIRunnable> gUMDRunnable = new GetUserMediaDevicesRunnable(
    onSuccess.forget(), onError.forget()
  );

  nsCOMPtr<nsIThread> deviceThread;
  nsresult rv = NS_NewThread(getter_AddRefs(deviceThread));
  NS_ENSURE_SUCCESS(rv, rv);


  deviceThread->Dispatch(gUMDRunnable, NS_DISPATCH_NORMAL);
  return NS_OK;
}

MediaEngine*
MediaManager::GetBackend()
{
  // Plugin backends as appropriate. The default engine also currently
  // includes picture support for Android.
  if (!mBackend) {
#if defined(MOZ_WEBRTC)
    mBackend = new MediaEngineWebRTC();
#else
    mBackend = new MediaEngineDefault();
#endif
  }

  return mBackend;
}

WindowTable*
MediaManager::GetActiveWindows()
{
  return &mActiveWindows;
}

void
MediaManager::OnNavigation(uint64_t aWindowID)
{
  // Invalidate this window. The runnables check this value before making
  // a call to content.
  {
    MutexAutoLock lock(mMutex);
    StreamListeners* listeners = mActiveWindows.Get(aWindowID);
    if (!listeners) {
      return;
    }

    uint32_t length = listeners->Length();
    for (uint32_t i = 0; i < length; i++) {
      nsRefPtr<GetUserMediaCallbackMediaStreamListener> listener =
        listeners->ElementAt(i);
      listener->Invalidate();
      listener = nullptr;
    }
    listeners->Clear();

    mActiveWindows.Remove(aWindowID);
  }
}

nsresult
MediaManager::Observe(nsISupports* aSubject, const char* aTopic,
  const PRUnichar* aData)
{
  NS_ASSERTION(NS_IsMainThread(), "Observer invoked off the main thread");
  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();

  if (!strcmp(aTopic, "xpcom-shutdown")) {
    obs->RemoveObserver(this, "xpcom-shutdown");
    obs->RemoveObserver(this, "getUserMedia:response:allow");
    obs->RemoveObserver(this, "getUserMedia:response:deny");

    // Close off any remaining active windows.
    {
      MutexAutoLock lock(mMutex);
      mActiveWindows.Clear();
      mActiveCallbacks.Clear();
      sSingleton = nullptr;
    }

    return NS_OK;
  }

  if (!strcmp(aTopic, "getUserMedia:response:allow")) {
    nsString key(aData);
    nsRefPtr<nsRunnable> runnable;
    {
      MutexAutoLock lock(mMutex);
      if (!mActiveCallbacks.Get(key, getter_AddRefs(runnable))) {
        return NS_OK;
      }
    }

    // Reuse the same thread to save memory.
    (void) MediaManager::GetThread();

    if (aSubject) {
      // A particular device was chosen by the user.
      // NOTE: does not allow setting a device to null; assumes nullptr
      nsCOMPtr<nsIMediaDevice> device = do_QueryInterface(aSubject);
      if (device) {
        GetUserMediaRunnable* gUMRunnable =
          static_cast<GetUserMediaRunnable*>(runnable.get());
        nsString type;
        device->GetType(type);
        if (type.EqualsLiteral("video")) {
          gUMRunnable->SetVideoDevice(static_cast<MediaDevice*>(device.get()));
        } else if (type.EqualsLiteral("audio")) {
          gUMRunnable->SetAudioDevice(static_cast<MediaDevice*>(device.get()));
        } else {
          NS_WARNING("Unknown device type in getUserMedia");
        }
      }
    }

    mMediaThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
    {
      MutexAutoLock lock(mMutex);
      mActiveCallbacks.Remove(key);
    }
    return NS_OK;
  }

  if (!strcmp(aTopic, "getUserMedia:response:deny")) {
    nsString key(aData);
    nsRefPtr<nsRunnable> runnable;
    {
      MutexAutoLock lock(mMutex);
      if (mActiveCallbacks.Get(key, getter_AddRefs(runnable))) {
        GetUserMediaRunnable* gUMRunnable =
          static_cast<GetUserMediaRunnable*>(runnable.get());
        gUMRunnable->Denied();
        mActiveCallbacks.Remove(key);
      }
    }
    return NS_OK;
  }

  return NS_OK;
}

} // namespace mozilla
