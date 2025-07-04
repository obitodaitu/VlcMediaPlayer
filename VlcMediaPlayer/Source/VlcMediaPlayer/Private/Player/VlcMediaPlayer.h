// Copyright 2024-2025, obitodaitu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Queue.h"
#include "IMediaCache.h"
#include "IMediaControls.h"
#include "IMediaPlayer.h"
#include "IMediaSamples.h"

#include "VlcMediaPlayerCallbacks.h"
#include "VlcMediaPlayerSource.h"
#include "VlcMediaPlayerTracks.h"
#include "VlcMediaPlayerView.h"

#include "VlcWrapper.h"

class IMediaEventSink;
class IMediaOutput;

enum class ELibvlcEventType;

//struct FLibvlcEvent;
//struct FLibvlcEventManager;
//struct FLibvlcInstance;
//struct FLibvlcMediaPlayer;


/**
 * Implements a media player using the Video LAN Codec (VLC) framework.
 */
class FVlcMediaPlayer
	: public IMediaPlayer
	, protected IMediaCache
	, protected IMediaControls
{
public:

	/**
	 * Create and initialize a new instance.
	 *
	 * @param InEventSink The object that receives media events from this player.
	 * @param InInstance The LibVLC instance to use.
	 */
	FVlcMediaPlayer(IMediaEventSink& InEventSink, libvlc_instance_t* InInstance);

	/** Virtual destructor. */
	virtual ~FVlcMediaPlayer();

public:

	//~ IMediaPlayer interface

	virtual void Close() override;
	virtual IMediaCache& GetCache() override;
	virtual IMediaControls& GetControls() override;
	virtual FString GetInfo() const override;
	virtual FName GetPlayerName() const;
	virtual FGuid GetPlayerPluginGUID() const;
	virtual IMediaSamples& GetSamples() override;
	virtual FString GetStats() const override;
	virtual IMediaTracks& GetTracks() override;
	virtual FString GetUrl() const override;
	virtual IMediaView& GetView() override;
	virtual bool Open(const FString& Url, const IMediaOptions* Options) override;
	virtual bool Open(const TSharedRef<FArchive, ESPMode::ThreadSafe>& Archive, const FString& OriginalUrl, const IMediaOptions* Options) override;
	virtual void TickInput(FTimespan DeltaTime, FTimespan Timecode) override;

protected:

	/**
	 * Initialize the media player.
	 *
	 * @return true on success, false otherwise.
	 */
	bool InitializePlayer();

protected:

	//~ IMediaControls interface

	virtual bool CanControl(EMediaControl Control) const override;
	virtual FTimespan GetDuration() const override;
	virtual float GetRate() const override;
	virtual EMediaState GetState() const override;
	virtual EMediaStatus GetStatus() const override;
	virtual TRangeSet<float> GetSupportedRates(EMediaRateThinning Thinning) const override;
	virtual FTimespan GetTime() const override;
	virtual bool IsLooping() const override;
	virtual bool Seek(const FTimespan& Time) override;
	virtual bool SetLooping(bool Looping) override;
	virtual bool SetRate(float Rate) override;

private:

	/** Handles event callbacks. */
	static void StaticEventCallback(const libvlc_event_t* Event, void* UserData);

private:

	/** VLC callback manager. */
	FVlcMediaPlayerCallbacks Callbacks;

	/** Current playback rate. */
	float CurrentRate;

	/** Current playback time (to work around VLC's broken time tracking). */
	FTimespan CurrentTime;

	/** The media event handler. */
	IMediaEventSink& EventSink;

	/** Collection of received player events. */
	TQueue<libvlc_event_e, EQueueMode::Mpsc> Events;

	/** Media information string. */
	FString Info;

	/** The media source (from URL or archive). */
	FVlcMediaPlayerSource MediaSource;

	/** The VLC media player object. */
	libvlc_media_player_t* Player;

	/** Whether playback should be looping. */
	bool ShouldLoop;

	/** Track collection. */
	FVlcMediaPlayerTracks Tracks;

	/** View settings. */
	FVlcMediaPlayerView View;
};
