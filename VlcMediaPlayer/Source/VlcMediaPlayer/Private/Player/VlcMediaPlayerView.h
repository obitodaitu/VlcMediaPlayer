// Copyright 2024-2025, obitodaitu. All Rights Reserved.

#pragma once

#include "IMediaView.h"

struct libvlc_media_player_t;
struct libvlc_video_viewpoint_t;


/**
 * Implements VLC video view settings.
 */
class FVlcMediaPlayerView
	: public IMediaView
{
public:

	/** Default constructor. */
	FVlcMediaPlayerView();

	/** Virtual destructor. */
	virtual ~FVlcMediaPlayerView();

public:

	/**
	 * Initialize this object for the specified VLC media player.
	 *
	 * @param InPlayer The VLC media player.
	 */
	void Initialize(libvlc_media_player_t& InPlayer);

	/** Shut down this object. */
	void Shutdown();

public:

	//~ IMediaView interface

	virtual bool GetViewField(float& OutHorizontal, float& OutVertical) const override;
	virtual bool GetViewOrientation(FQuat& OutOrientation) const override;
	virtual bool SetViewField(float Horizontal, float Vertical, bool Absolute) override;
	virtual bool SetViewOrientation(const FQuat& Orientation, bool Absolute) override;

private:

	/** The current field of view (horizontal & vertical). */
	float CurrentFieldOfView;

	/** The current view orientation. */
	FQuat CurrentOrientation;

	/** The VLC media player object. */
	libvlc_media_player_t* Player;

	/** The VLC video viewpoint. */
	libvlc_video_viewpoint_t* Viewpoint;
};
