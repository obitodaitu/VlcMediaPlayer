// Copyright 2024-2025, obitodaitu. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "IMediaTracks.h"
#include "Internationalization/Text.h"

#include "VlcWrapper.h"


/**
 * Implements the track collection for VLC based media players.
 */
class FVlcMediaPlayerTracks
	: public IMediaTracks
{
	struct FTrack
	{
		FText DisplayName;
		int32 Id;
		FString Name;
	}; 

public:

	/** Default constructor. */
	FVlcMediaPlayerTracks();

public:

	/**
	 * Initialize this object for the specified VLC media player.
	 *
	 * @param InPlayer The VLC media player.
	 * @param OutInfo Will contain information about the available media tracks.
	 */
	void Initialize(libvlc_media_player_t& InPlayer, FString& OutInfo);

	/** Shut down this object. */
	void Shutdown();

public:

	//~ IMediaTracks interface

	virtual bool GetAudioTrackFormat(int32 TrackIndex, int32 FormatIndex, FMediaAudioTrackFormat& OutFormat) const override;
	virtual int32 GetNumTracks(EMediaTrackType TrackType) const override;
	virtual int32 GetNumTrackFormats(EMediaTrackType TrackType, int32 TrackIndex) const override;
	virtual int32 GetSelectedTrack(EMediaTrackType TrackType) const override;
	virtual FText GetTrackDisplayName(EMediaTrackType TrackType, int32 TrackIndex) const override;
	virtual int32 GetTrackFormat(EMediaTrackType TrackType, int32 TrackIndex) const override;
	virtual FString GetTrackLanguage(EMediaTrackType TrackType, int32 TrackIndex) const override;
	virtual FString GetTrackName(EMediaTrackType TrackType, int32 TrackIndex) const override;
	virtual bool GetVideoTrackFormat(int32 TrackIndex, int32 FormatIndex, FMediaVideoTrackFormat& OutFormat) const override;
	virtual bool SelectTrack(EMediaTrackType TrackType, int32 TrackIndex) override;
	virtual bool SetTrackFormat(EMediaTrackType TrackType, int32 TrackIndex, int32 FormatIndex) override;

private:

	/** Audio track descriptors. */
	TArray<FTrack> AudioTracks;

	/** Caption track descriptors. */
	TArray<FTrack> CaptionTracks;

	/** The VLC media player object. */
	libvlc_media_player_t* Player;

	/** Video track descriptors. */
	TArray<FTrack> VideoTracks;
};
