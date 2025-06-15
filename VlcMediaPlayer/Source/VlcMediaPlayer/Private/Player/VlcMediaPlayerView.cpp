// Copyright 2024-2025, obitodaitu. All Rights Reserved.

#include "VlcMediaPlayerView.h"
#include "VlcMediaPlayerPrivate.h"

#include "VlcWrapper.h"


/* FVlcMediaPlayerView structors
*****************************************************************************/

FVlcMediaPlayerView::FVlcMediaPlayerView()
	: CurrentFieldOfView(90.0f)
	, CurrentOrientation(EForceInit::ForceInit)
	, Player(nullptr)
{
	Viewpoint = libvlc_video_new_viewpoint();

	if (Viewpoint == nullptr)
	{
		UE_LOG(LogVlcMediaPlayer, Warning, TEXT("Failed to create viewpoint structure; 360 view controls will be unavailable."));
	}
}


FVlcMediaPlayerView::~FVlcMediaPlayerView()
{
	if (Viewpoint != nullptr)
	{
		libvlc_free(Viewpoint);
		Viewpoint = nullptr;
	}
}


/* FVlcMediaPlayerView interface
*****************************************************************************/

void FVlcMediaPlayerView::Initialize(libvlc_media_player_t& InPlayer)
{
	Player = &InPlayer;
}


void FVlcMediaPlayerView::Shutdown()
{
	Player = nullptr;
}


/* IMediaView interface
*****************************************************************************/

bool FVlcMediaPlayerView::GetViewField(float& OutHorizontal, float& OutVertical) const
{
	OutHorizontal = CurrentFieldOfView;
	OutVertical = CurrentFieldOfView;

	return true;
}


bool FVlcMediaPlayerView::GetViewOrientation(FQuat& OutOrientation) const
{
	OutOrientation = CurrentOrientation;

	return true;
}


bool FVlcMediaPlayerView::SetViewField(float Horizontal, float Vertical, bool Absolute)
{
	if ((Player == nullptr) || (Viewpoint == nullptr))
	{
		return false;
	}

	if (!Absolute)
	{
		Horizontal = CurrentFieldOfView + Horizontal;
	}

	FVector Euler = CurrentOrientation.Euler();

	Viewpoint->f_roll = Euler.X;
	Viewpoint->f_pitch = Euler.Y;
	Viewpoint->f_yaw = Euler.Z;
	Viewpoint->f_field_of_view = FMath::ClampAngle(Horizontal, 10.0f, 360.0f);

	UE_LOG(LogVlcMediaPlayer, VeryVerbose, TEXT("Setting viewpoint to %f %f %f / %f."), Viewpoint->f_roll, Viewpoint->f_pitch, Viewpoint->f_yaw, Viewpoint->f_field_of_view);

	if (libvlc_video_update_viewpoint(Player, Viewpoint, true) != 0)
	{
		return false;
	}

	CurrentFieldOfView = Horizontal;

	return true;
}


bool FVlcMediaPlayerView::SetViewOrientation(const FQuat& Orientation, bool Absolute)
{
	if ((Player == nullptr) || (Viewpoint == nullptr))
	{
		return false;
	}

	FQuat NewOrientation;

	if (Absolute)
	{
		NewOrientation = Orientation;
	}
	else
	{
		NewOrientation = Orientation * CurrentOrientation;
	}

	FVector Euler = NewOrientation.Euler();

	Viewpoint->f_roll = Euler.X;
	Viewpoint->f_pitch = Euler.Y;
	Viewpoint->f_yaw = Euler.Z;
	Viewpoint->f_field_of_view = CurrentFieldOfView;

	UE_LOG(LogVlcMediaPlayer, VeryVerbose, TEXT("Setting viewpoint to %f %f %f / %f."), Viewpoint->f_roll, Viewpoint->f_pitch, Viewpoint->f_yaw, Viewpoint->f_field_of_view);

	if (libvlc_video_update_viewpoint(Player, Viewpoint, true) != 0)
	{
		return false;
	}

	CurrentOrientation = NewOrientation;

	return true;
}
