// Copyright 2024-2025, obitodaitu. All Rights Reserved.

#include "VlcMediaPlayer.h"
#include "VlcMediaPlayerPrivate.h"

#include "IMediaEventSink.h"
#include "IMediaOptions.h"
#include "Misc/FileHelper.h"
#include "Serialization/ArrayReader.h"


/* FVlcMediaPlayer structors
 *****************************************************************************/

FVlcMediaPlayer::FVlcMediaPlayer(IMediaEventSink& InEventSink, libvlc_instance_t* InVlcInstance)
	: CurrentRate(0.0f)
	, CurrentTime(FTimespan::Zero())
	, EventSink(InEventSink)
	, MediaSource(InVlcInstance)
	, Player(nullptr)
	, ShouldLoop(false)
{ }


FVlcMediaPlayer::~FVlcMediaPlayer()
{
	Close();
}


/* IMediaControls interface
 *****************************************************************************/

bool FVlcMediaPlayer::CanControl(EMediaControl Control) const
{
	if (Player == nullptr)
	{
		return false;
	}

	if (Control == EMediaControl::Pause)
	{
		return (libvlc_media_player_can_pause(Player) != 0);
	}

	if (Control == EMediaControl::Resume)
	{
		return (libvlc_media_player_get_state(Player) != libvlc_state_t::libvlc_Playing);
	}

	if ((Control == EMediaControl::Scrub) || (Control == EMediaControl::Seek))
	{
		return (libvlc_media_player_is_seekable(Player) != 0);
	}

	return false;
}


FTimespan FVlcMediaPlayer::GetDuration() const
{
	return MediaSource.GetDuration();
}


float FVlcMediaPlayer::GetRate() const
{
	return CurrentRate;
}


EMediaState FVlcMediaPlayer::GetState() const
{
	if (Player == nullptr)
	{
		return EMediaState::Closed;
	}

	libvlc_state_t State = libvlc_media_player_get_state(Player);

	switch (State)
	{
	case libvlc_state_t::libvlc_Error:
		return EMediaState::Error;

	case libvlc_state_t::libvlc_Buffering:
	case libvlc_state_t::libvlc_Opening:
		return EMediaState::Preparing;

	case libvlc_state_t::libvlc_Paused:
		return EMediaState::Paused;

	case libvlc_state_t::libvlc_Playing:
		return EMediaState::Playing;

	case libvlc_state_t::libvlc_Ended:
	case libvlc_state_t::libvlc_NothingSpecial:
	case libvlc_state_t::libvlc_Stopped:
		return EMediaState::Stopped;
	}

	return EMediaState::Error; // should never get here
}


EMediaStatus FVlcMediaPlayer::GetStatus() const
{
	return (GetState() == EMediaState::Preparing) ? EMediaStatus::Buffering : EMediaStatus::None;
}


TRangeSet<float> FVlcMediaPlayer::GetSupportedRates(EMediaRateThinning Thinning) const
{
	TRangeSet<float> Result;

	if (Thinning == EMediaRateThinning::Thinned)
	{
		Result.Add(TRange<float>::Inclusive(0.0f, 10.0f));
	}
	else
	{
		Result.Add(TRange<float>::Inclusive(0.0f, 1.0f));
	}

	return Result;
}


FTimespan FVlcMediaPlayer::GetTime() const
{
	return CurrentTime;
}


bool FVlcMediaPlayer::IsLooping() const
{
	return ShouldLoop;
}


bool FVlcMediaPlayer::Seek(const FTimespan& Time)
{
	libvlc_state_t State = libvlc_media_player_get_state(Player);

	if ((State == libvlc_state_t::libvlc_Opening) ||
		(State == libvlc_state_t::libvlc_Buffering) ||
		(State == libvlc_state_t::libvlc_Error))
	{
		return false;
	}

	if (Time != CurrentTime)
	{
		libvlc_media_player_set_time(Player, Time.GetTotalMilliseconds());
		CurrentTime = Time;
	}

	return true;
}


bool FVlcMediaPlayer::SetLooping(bool Looping)
{
	ShouldLoop = Looping;
	return true;
}


bool FVlcMediaPlayer::SetRate(float Rate)
{
	if (Player == nullptr)
	{
		return false;
	}

	if ((libvlc_media_player_set_rate(Player, Rate) == -1))
	{
		return false;
	}

	if (FMath::IsNearlyZero(Rate))
	{
		if (libvlc_media_player_get_state(Player) == libvlc_state_t::libvlc_Playing)
		{
			if (libvlc_media_player_can_pause(Player) == 0)
			{
				return false;
			}

			libvlc_media_player_pause(Player);
		}
	}
	else if (libvlc_media_player_get_state(Player) != libvlc_state_t::libvlc_Playing)
	{
		if (libvlc_media_player_play(Player) == -1)
		{
			return false;
		}
	}

	return true;
}


/* IMediaPlayer interface
 *****************************************************************************/

void FVlcMediaPlayer::Close()
{
	if (Player == nullptr)
	{
		return;
	}

	// detach callback handlers
	Callbacks.Shutdown();
	Tracks.Shutdown();
	View.Shutdown();

	// release player
	libvlc_media_player_stop(Player);
	libvlc_media_player_release(Player);
	Player = nullptr;

	// reset fields
	CurrentRate = 0.0f;
	CurrentTime = FTimespan::Zero();
	MediaSource.Close();
	Info.Empty();

	// notify listeners
	EventSink.ReceiveMediaEvent(EMediaEvent::TracksChanged);
	EventSink.ReceiveMediaEvent(EMediaEvent::MediaClosed);
}


IMediaCache& FVlcMediaPlayer::GetCache()
{
	return *this;
}


IMediaControls& FVlcMediaPlayer::GetControls() 
{
	return *this;
}


FString FVlcMediaPlayer::GetInfo() const
{
	return Info;
}


FName FVlcMediaPlayer::GetPlayerName() const
{
	static FName PlayerName(TEXT("VlcMedia"));
	return PlayerName;
}

FGuid FVlcMediaPlayer::GetPlayerPluginGUID() const
{
	static FGuid GetPlayerPluginGUID(TEXT("VlcMedia"));
	return GetPlayerPluginGUID;
}


IMediaSamples& FVlcMediaPlayer::GetSamples()
{
	return Callbacks.GetSamples();
}


FString FVlcMediaPlayer::GetStats() const
{
	libvlc_media_t* Media = MediaSource.GetMedia();

	if (Media == nullptr)
	{
		return TEXT("No media opened.");
	}

	libvlc_media_stats_t Stats;
	
	if (!libvlc_media_get_stats(Media, &Stats))
	{
		return TEXT("Stats currently not available.");
	}

	FString StatsString;
	{
		StatsString += TEXT("General\n");
		StatsString += FString::Printf(TEXT("    Decoded Video: %i\n"), Stats.i_decoded_video);
		StatsString += FString::Printf(TEXT("    Decoded Audio: %i\n"), Stats.i_decoded_audio);
		StatsString += FString::Printf(TEXT("    Displayed Pictures: %i\n"), Stats.i_displayed_pictures);
		StatsString += FString::Printf(TEXT("    Lost Pictures: %i\n"), Stats.i_lost_pictures);
		StatsString += FString::Printf(TEXT("    Played A-Buffers: %i\n"), Stats.i_played_abuffers);
		StatsString += FString::Printf(TEXT("    Lost Lost A-Buffers: %i\n"), Stats.i_lost_abuffers);
		StatsString += TEXT("\n");

		StatsString += TEXT("Input\n");
		StatsString += FString::Printf(TEXT("    Bit Rate: %i\n"), Stats.f_input_bitrate);
		StatsString += FString::Printf(TEXT("    Bytes Read: %i\n"), Stats.i_read_bytes);
		StatsString += TEXT("\n");

		StatsString += TEXT("Demux\n");
		StatsString += FString::Printf(TEXT("    Bit Rate: %f\n"), Stats.f_demux_bitrate);
		StatsString += FString::Printf(TEXT("    Bytes Read: %i\n"), Stats.i_demux_read_bytes);
		StatsString += FString::Printf(TEXT("    Corrupted: %i\n"), Stats.i_demux_corrupted);
		StatsString += FString::Printf(TEXT("    Discontinuity: %i\n"), Stats.i_demux_discontinuity);
		StatsString += TEXT("\n");

		StatsString += TEXT("Network\n");
		StatsString += FString::Printf(TEXT("    Bitrate: %f\n"), Stats.f_send_bitrate);
		StatsString += FString::Printf(TEXT("    Sent Bytes: %i\n"), Stats.i_sent_bytes);
		StatsString += FString::Printf(TEXT("    Sent Packets: %i\n"), Stats.i_sent_packets);
		StatsString += TEXT("\n");
	}

	return StatsString;
}


IMediaTracks& FVlcMediaPlayer::GetTracks()
{
	return Tracks;
}


FString FVlcMediaPlayer::GetUrl() const
{
	return MediaSource.GetCurrentUrl();
}


IMediaView& FVlcMediaPlayer::GetView()
{
	return View;
}


bool FVlcMediaPlayer::Open(const FString& Url, const IMediaOptions* Options)
{
	Close();

	if (Url.IsEmpty())
	{
		return false;
	}

	if (Url.StartsWith(TEXT("file://")))
	{
		// open local files via platform file system
		TSharedPtr<FArchive, ESPMode::ThreadSafe> Archive;
		const TCHAR* FilePath = &Url[7];

		if ((Options != nullptr) && Options->GetMediaOption("PrecacheFile", false))
		{
			FArrayReader* Reader = new FArrayReader;

			if (FFileHelper::LoadFileToArray(*Reader, FilePath))
			{
				Archive = MakeShareable(Reader);
			}
			else
			{
				delete Reader;
			}
		}
		else
		{
			Archive = MakeShareable(IFileManager::Get().CreateFileReader(FilePath));
		}

		if (!Archive.IsValid())
		{
			UE_LOG(LogVlcMediaPlayer, Warning, TEXT("Failed to open media file: %s"), FilePath);
			return false;
		}

		if (!MediaSource.OpenArchive(Archive.ToSharedRef(), Url))
		{
			return false;
		}
	}
	else if (!MediaSource.OpenUrl(Url))
	{
		return false;
	}

	return InitializePlayer();
}


bool FVlcMediaPlayer::Open(const TSharedRef<FArchive, ESPMode::ThreadSafe>& Archive, const FString& OriginalUrl, const IMediaOptions* /*Options*/)
{
	Close();

	if (OriginalUrl.IsEmpty() || !MediaSource.OpenArchive(Archive, OriginalUrl))
	{
		return false;
	}
	
	return InitializePlayer();
}


void FVlcMediaPlayer::TickInput(FTimespan DeltaTime, FTimespan /*Timecode*/)
{
	if (Player == nullptr)
	{
		return;
	}

	// process events
	libvlc_event_e Event;

	while (Events.Dequeue(Event))
	{
		switch (Event)
		{
		case libvlc_event_e::libvlc_MediaMetaChanged:
			EventSink.ReceiveMediaEvent(EMediaEvent::MetadataChanged);
			break;

		case libvlc_event_e::libvlc_MediaPlayerBuffering:
			EventSink.ReceiveMediaEvent(EMediaEvent::MediaBuffering);
			break;

		
		case libvlc_event_e::libvlc_MediaParsedChanged:
			Callbacks.Initialize(*Player);
			EventSink.ReceiveMediaEvent(EMediaEvent::TracksChanged);
			break;

		case libvlc_event_e::libvlc_MediaPlayerOpening:
			EventSink.ReceiveMediaEvent(EMediaEvent::MediaOpened);
			break;

		case libvlc_event_e::libvlc_MediaPlayerEndReached:
			libvlc_media_player_stop(Player);
			
			Callbacks.GetSamples().FlushSamples();
			EventSink.ReceiveMediaEvent(EMediaEvent::PlaybackEndReached);

			if (ShouldLoop && (CurrentRate != 0.0f))
			{
				CurrentTime = FTimespan::Zero();
				SetRate(CurrentRate);
			}
			else
			{
				EventSink.ReceiveMediaEvent(EMediaEvent::PlaybackSuspended);
			}
			break;

		case libvlc_event_e::libvlc_MediaPlayerPaused:
			EventSink.ReceiveMediaEvent(EMediaEvent::PlaybackSuspended);
			break;

		case libvlc_event_e::libvlc_MediaPlayerPlaying:
			EventSink.ReceiveMediaEvent(EMediaEvent::PlaybackResumed);
			break;

		case libvlc_event_e::libvlc_MediaPlayerPositionChanged:
			EventSink.ReceiveMediaEvent(EMediaEvent::SeekCompleted);
			break;

		default:
			continue;
		}
	}

	const libvlc_state_t State = libvlc_media_player_get_state(Player);

	// update current time & rate
	if (State == libvlc_state_t::libvlc_Playing)
	{
		CurrentRate = libvlc_media_player_get_rate(Player);
		CurrentTime += DeltaTime * CurrentRate;
	}
	else
	{
		CurrentRate = 0.0f;
	}

	Callbacks.SetCurrentTime(CurrentTime);
}


/* FVlcMediaPlayer implementation
 *****************************************************************************/

bool FVlcMediaPlayer::InitializePlayer()
{
	// create player for media source
	Player = libvlc_media_player_new_from_media(MediaSource.GetMedia());

	if (Player == nullptr)
	{
		UE_LOG(LogVlcMediaPlayer, Warning, TEXT("Failed to initialize media player: %s"), ANSI_TO_TCHAR(libvlc_errmsg()));
		return false;
	}

	Tracks.Initialize(*Player, Info);
	View.Initialize(*Player);
	
	// attach to event managers
	libvlc_event_manager_t* MediaEventManager = libvlc_media_event_manager(MediaSource.GetMedia());
	libvlc_event_manager_t* PlayerEventManager = libvlc_media_player_event_manager(Player);

	if ((MediaEventManager == nullptr) || (PlayerEventManager == nullptr))
	{
		libvlc_media_player_release(Player);
		Player = nullptr;

		return false;
	}

	libvlc_event_attach(MediaEventManager, libvlc_event_e::libvlc_MediaParsedChanged, &FVlcMediaPlayer::StaticEventCallback, this);
	libvlc_event_attach(PlayerEventManager, libvlc_event_e::libvlc_MediaPlayerEndReached, &FVlcMediaPlayer::StaticEventCallback, this);
	libvlc_event_attach(PlayerEventManager, libvlc_event_e::libvlc_MediaPlayerPlaying, &FVlcMediaPlayer::StaticEventCallback, this);
	libvlc_event_attach(PlayerEventManager, libvlc_event_e::libvlc_MediaPlayerPositionChanged, &FVlcMediaPlayer::StaticEventCallback, this);
	libvlc_event_attach(PlayerEventManager, libvlc_event_e::libvlc_MediaPlayerStopped, &FVlcMediaPlayer::StaticEventCallback, this);
	libvlc_event_attach(PlayerEventManager, libvlc_event_e::libvlc_MediaMetaChanged, &FVlcMediaPlayer::StaticEventCallback, this);
	libvlc_event_attach(PlayerEventManager, libvlc_event_e::libvlc_MediaPlayerBuffering, &FVlcMediaPlayer::StaticEventCallback, this);
	libvlc_event_attach(PlayerEventManager, libvlc_event_e::libvlc_MediaPlayerOpening, &FVlcMediaPlayer::StaticEventCallback, this);

	// initialize player
	CurrentRate = 0.0f;
	CurrentTime = FTimespan::Zero();

	EventSink.ReceiveMediaEvent(EMediaEvent::MediaOpened);

	return true;
}


/* FVlcMediaPlayer static functions
 *****************************************************************************/

void FVlcMediaPlayer::StaticEventCallback(const libvlc_event_t* Event, void* UserData)
{
	if (Event == nullptr)
	{
		return;
	}

	UE_LOG(LogVlcMediaPlayer, Verbose, TEXT("Player %p: Event [%s]"), UserData, UTF8_TO_TCHAR(libvlc_event_type_name(Event->type)));
	
	if (UserData != nullptr)
	{
		((FVlcMediaPlayer*)UserData)->Events.Enqueue(static_cast<libvlc_event_e>(Event->type));
	}
}
