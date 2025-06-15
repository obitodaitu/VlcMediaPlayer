// Copyright 2024-2025, obitodaitu. All Rights Reserved.

#include "VlcMediaPlayerTracks.h"
#include "VlcMediaPlayerPrivate.h"
#include "MediaHelpers.h"


#define LOCTEXT_NAMESPACE "FVlcMediaPlayerTracks"


/* FVlcMediaPlayerTracks structors
*****************************************************************************/

FVlcMediaPlayerTracks::FVlcMediaPlayerTracks()
	: Player(nullptr)
{ }


/* FVlcMediaPlayerTracks interface
*****************************************************************************/

void FVlcMediaPlayerTracks::Initialize(libvlc_media_player_t& InPlayer, FString& OutInfo)
{
	Shutdown();

	UE_LOG(LogVlcMediaPlayer, Verbose, TEXT("Tracks: %p: Initializing tracks"), this);

	Player = &InPlayer;

	int32 Width = libvlc_video_get_width(Player);
	int32 Height = libvlc_video_get_height(Player);
	int32 StreamCount = 0;

	// @todo gmp: fix audio specs
	libvlc_audio_set_format(Player, "S16N", 44100, 2);
	libvlc_video_set_format(Player, "RV32", Width, Height, Width * 4);

	// initialize audio tracks
	libvlc_track_description_t* AudioTrackDescr = libvlc_audio_get_track_description(Player);
	{
		while (AudioTrackDescr != nullptr)
		{
			if (AudioTrackDescr->i_id != -1)
			{
				FTrack Track;
				{
					Track.Id = AudioTrackDescr->i_id;
					Track.Name = ANSI_TO_TCHAR(AudioTrackDescr->psz_name);
					Track.DisplayName = Track.Name.IsEmpty()
						? FText::Format(LOCTEXT("AudioTrackFormat", "Audio Track {0}"), FText::AsNumber(AudioTracks.Num()))
						: FText::FromString(Track.Name);
				}

				AudioTracks.Add(Track);

				OutInfo += FString::Printf(TEXT("Stream %i\n"), StreamCount);
				OutInfo += TEXT("    Type: Audio\n");
				OutInfo += FString::Printf(TEXT("    Name: %s\n"), *Track.Name);
				OutInfo += TEXT("\n");

				++StreamCount;
			}

			AudioTrackDescr = AudioTrackDescr->p_next;
		}
	}
	libvlc_track_description_release(AudioTrackDescr);

	// initialize caption tracks
	libvlc_track_description_t* CaptionTrackDescr = libvlc_video_get_spu_description(Player);
	{
		while (CaptionTrackDescr != nullptr)
		{
			if (CaptionTrackDescr->i_id != -1)
			{
				FTrack Track;
				{
					Track.Id = CaptionTrackDescr->i_id;
					Track.Name = ANSI_TO_TCHAR(CaptionTrackDescr->psz_name);
					Track.DisplayName = Track.Name.IsEmpty()
						? FText::Format(LOCTEXT("CaptionTrackFormat", "Caption Track {0}"), FText::AsNumber(CaptionTracks.Num()))
						: FText::FromString(Track.Name);
				}

				CaptionTracks.Add(Track);
				
				OutInfo += FString::Printf(TEXT("Stream %i\n"), StreamCount);
				OutInfo += TEXT("    Type: Caption\n");
				OutInfo += FString::Printf(TEXT("    Name: %s\n"), *Track.Name);
				OutInfo += TEXT("\n");

				++StreamCount;
			}

			CaptionTrackDescr = CaptionTrackDescr->p_next;
		}
	}
	libvlc_track_description_release(CaptionTrackDescr);

	// initialize video tracks
	libvlc_track_description_t* VideoTrackDescr = libvlc_video_get_track_description(Player);
	{
		while (VideoTrackDescr != nullptr)
		{
			if (VideoTrackDescr->i_id != -1)
			{
				FTrack Track;
				{
					Track.Id = VideoTrackDescr->i_id;
					Track.Name = ANSI_TO_TCHAR(VideoTrackDescr->psz_name);
					Track.DisplayName = Track.Name.IsEmpty()
						? FText::Format(LOCTEXT("VideoTrackFormat", "Video Track {0}"), FText::AsNumber(VideoTracks.Num()))
						: FText::FromString(Track.Name);
				}

				VideoTracks.Add(Track);

				OutInfo += FString::Printf(TEXT("Stream %i\n"), StreamCount);
				OutInfo += TEXT("    Type: Video\n");
				OutInfo += FString::Printf(TEXT("    Name: %s\n"), *Track.Name);
				OutInfo += TEXT("\n");

				++StreamCount;
			}

			VideoTrackDescr = VideoTrackDescr->p_next;
		}
	}
	libvlc_track_description_release(VideoTrackDescr);

	UE_LOG(LogVlcMediaPlayer, Verbose, TEXT("Tracks %p: Found %i streams"), this, StreamCount);
}


void FVlcMediaPlayerTracks::Shutdown()
{
	UE_LOG(LogVlcMediaPlayer, Verbose, TEXT("Tracks: %p: Shutting down tracks"), this);

	if (Player != nullptr)
	{
		AudioTracks.Reset();
		VideoTracks.Reset();
		Player = nullptr;
	}
}


/* IMediaTracks interface
*****************************************************************************/

bool FVlcMediaPlayerTracks::GetAudioTrackFormat(int32 TrackIndex, int32 FormatIndex, FMediaAudioTrackFormat& OutFormat) const
{
	if (!AudioTracks.IsValidIndex(TrackIndex) || (FormatIndex != 0))
	{
		return false;
	}

	// @todo gmp: fix audio format
	OutFormat.BitsPerSample = 0;
	OutFormat.NumChannels = 2;
	OutFormat.SampleRate = 44100;
	OutFormat.TypeName = TEXT("PCM");

	return true;
}


int32 FVlcMediaPlayerTracks::GetNumTracks(EMediaTrackType TrackType) const
{
	switch (TrackType)
	{
	case EMediaTrackType::Audio:
		return AudioTracks.Num();

	case EMediaTrackType::Caption:
		return CaptionTracks.Num();

	case EMediaTrackType::Video:
		return VideoTracks.Num();

	default:
		return 0;
	}
}


int32 FVlcMediaPlayerTracks::GetNumTrackFormats(EMediaTrackType TrackType, int32 TrackIndex) const
{
	return ((TrackIndex >= 0) && (TrackIndex < GetNumTracks(TrackType))) ? 1 : 0;
}


int32 FVlcMediaPlayerTracks::GetSelectedTrack(EMediaTrackType TrackType) const
{
	if (Player == nullptr)
	{
		return INDEX_NONE;
	}

	const TArray<FTrack>* Tracks = nullptr;
	int32 TrackId = INDEX_NONE;

	switch (TrackType)
	{
	case EMediaTrackType::Audio:
		Tracks = &AudioTracks;
		TrackId = libvlc_audio_get_track(Player);
		break;

	case EMediaTrackType::Caption:
		Tracks = &CaptionTracks;
		TrackId = libvlc_video_get_spu(Player);
		break;

	case EMediaTrackType::Video:
		Tracks = &VideoTracks;
		TrackId = libvlc_video_get_track(Player);
		break;
	}

	if (TrackId != INDEX_NONE)
	{
		check(Tracks != nullptr);

		for (int32 TrackIndex = 0; TrackIndex < Tracks->Num(); ++TrackIndex)
		{
			if ((*Tracks)[TrackIndex].Id == TrackId)
			{
				return TrackIndex;
			}
		}
	}

	return INDEX_NONE;
}


FText FVlcMediaPlayerTracks::GetTrackDisplayName(EMediaTrackType TrackType, int32 TrackIndex) const
{
	switch (TrackType)
	{
	case EMediaTrackType::Audio:
		if (AudioTracks.IsValidIndex(TrackIndex))
		{
			return AudioTracks[TrackIndex].DisplayName;
		}
		break;

	case EMediaTrackType::Caption:
		if (CaptionTracks.IsValidIndex(TrackIndex))
		{
			return CaptionTracks[TrackIndex].DisplayName;
		}
		break;

	case EMediaTrackType::Video:
		if (VideoTracks.IsValidIndex(TrackIndex))
		{
			return VideoTracks[TrackIndex].DisplayName;
		}

	default:
		break;
	}

	return FText::GetEmpty();
}


int32 FVlcMediaPlayerTracks::GetTrackFormat(EMediaTrackType TrackType, int32 TrackIndex) const
{
	return (GetSelectedTrack(TrackType) != INDEX_NONE) ? 0 : INDEX_NONE;
}


FString FVlcMediaPlayerTracks::GetTrackLanguage(EMediaTrackType TrackType, int32 TrackIndex) const
{
	return TEXT("und"); // libvlc currently doesn't provide language codes
}


FString FVlcMediaPlayerTracks::GetTrackName(EMediaTrackType TrackType, int32 TrackIndex) const
{
	switch (TrackType)
	{
	case EMediaTrackType::Audio:
		if (AudioTracks.IsValidIndex(TrackIndex))
		{
			return AudioTracks[TrackIndex].Name;
		}
		break;

	case EMediaTrackType::Caption:
		if (CaptionTracks.IsValidIndex(TrackIndex))
		{
			return CaptionTracks[TrackIndex].Name;
		}
		break;

	case EMediaTrackType::Video:
		if (VideoTracks.IsValidIndex(TrackIndex))
		{
			return VideoTracks[TrackIndex].Name;
		}

	default:
		break;
	}

	return FString();
}


bool FVlcMediaPlayerTracks::GetVideoTrackFormat(int32 TrackIndex, int32 FormatIndex, FMediaVideoTrackFormat& OutFormat) const
{
	if (!VideoTracks.IsValidIndex(TrackIndex) || (FormatIndex != 0) || (Player == nullptr))
	{
		return false;
	}

	// @todo gmp: fix video specs
	OutFormat.Dim = FIntPoint(libvlc_video_get_width(Player), libvlc_video_get_height(Player));
	OutFormat.FrameRate = libvlc_media_player_get_fps(Player);
	OutFormat.FrameRates = TRange<float>(OutFormat.FrameRate);
	OutFormat.TypeName = TEXT("Default");

	return true;
}


bool FVlcMediaPlayerTracks::SelectTrack(EMediaTrackType TrackType, int32 TrackIndex)
{
	if (Player == nullptr)
	{
		return false; // not initialized
	}

	UE_LOG(LogVlcMediaPlayer, Verbose, TEXT("Tracks %p: Selecting %s track %i"), this, *MediaUtils::TrackTypeToString(TrackType), TrackIndex);

	int32 TrackId = INDEX_NONE;

	switch (TrackType)
	{
	case EMediaTrackType::Audio:
		if (AudioTracks.IsValidIndex(TrackIndex))
		{
			TrackId = AudioTracks[TrackIndex].Id;
		}
		else if (TrackIndex == INDEX_NONE)
		{
			TrackId = -1;
		}
		else
		{
			return false; // invalid track
		}

		if (libvlc_audio_set_track(Player, TrackId) != 0)
		{
			UE_LOG(LogVlcMediaPlayer, Verbose, TEXT("Tracks %p: Failed to %s audio track %i (id %i)"), this, (TrackId == -1) ? TEXT("disable") : TEXT("enable"), TrackIndex, TrackId);
			return false;
		}

	case EMediaTrackType::Caption:
		if (CaptionTracks.IsValidIndex(TrackIndex))
		{
			TrackId = CaptionTracks[TrackIndex].Id;
		}
		else if (TrackIndex == INDEX_NONE)
		{
			TrackId = -1;
		}
		else
		{
			return false; // invalid track
		}

		if (libvlc_video_set_spu(Player, TrackId) != 0)
		{
			UE_LOG(LogVlcMediaPlayer, Verbose, TEXT("Tracks %p: Failed to %s caption track %i (id %i)"), this, (TrackId == -1) ? TEXT("disable") : TEXT("enable"), TrackIndex, TrackId);
			return false;
		}

	case EMediaTrackType::Video:
		if (VideoTracks.IsValidIndex(TrackIndex))
		{
			TrackId = VideoTracks[TrackIndex].Id;
		}
		else if (TrackIndex == INDEX_NONE)
		{
			TrackId = -1;
		}
		else
		{
			return false; // invalid track
		}

		if ((TrackId != -1) && (libvlc_video_set_track(Player, -1) != 0))
		{
			UE_LOG(LogVlcMediaPlayer, Verbose, TEXT("Tracks %p: Failed to disable video decoding"), this);
			return false;
		}

		if (libvlc_video_set_track(Player, TrackId) != 0)
		{
			UE_LOG(LogVlcMediaPlayer, Verbose, TEXT("Tracks %p: Failed to %s video track %i (id %i)"), this, (TrackId == -1) ? TEXT("disable") : TEXT("enable"), TrackIndex, TrackId);
			return false;
		}

	default:
		return false; // unsupported track type
	}
}


bool FVlcMediaPlayerTracks::SetTrackFormat(EMediaTrackType TrackType, int32 TrackIndex, int32 FormatIndex)
{
	if ((Player == nullptr) || (FormatIndex != 0))
	{
		return false;
	}

	switch (TrackType)
	{
	case EMediaTrackType::Audio:
		return AudioTracks.IsValidIndex(TrackIndex);

	case EMediaTrackType::Caption:
		return CaptionTracks.IsValidIndex(TrackIndex);

	case EMediaTrackType::Video:
		return VideoTracks.IsValidIndex(TrackIndex);

	default:
		return false;
	}
}


#undef LOCTEXT_NAMESPACE
