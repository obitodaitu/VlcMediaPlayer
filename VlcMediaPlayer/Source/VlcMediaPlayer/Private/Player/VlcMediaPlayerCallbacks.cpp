// Copyright 2024-2025, obitodaitu. All Rights Reserved.

#include "VlcMediaPlayerCallbacks.h"
#include "VlcMediaPlayerPrivate.h"

#include "IMediaAudioSample.h"
#include "IMediaOptions.h"
#include "IMediaTextureSample.h"
#include "MediaSamples.h"

#include "VlcMediaPlayerAudioSample.h"
#include "VlcMediaPlayerTextureSample.h"

#include "VlcWrapper.h"

/* FVlcMediaOutput structors
 *****************************************************************************/

FVlcMediaPlayerCallbacks::FVlcMediaPlayerCallbacks()
	: AudioChannels(0)
	, AudioSampleFormat(EMediaAudioSampleFormat::Int16)
	, AudioSamplePool(new FVlcMediaAudioSamplePool)
	, AudioSampleRate(0)
	, AudioSampleSize(0)
	, CurrentTime(FTimespan::Zero())
	, Player(nullptr)
	, Samples(new FMediaSamples)
	, VideoBufferDim(FIntPoint::ZeroValue)
	, VideoBufferStride(0)
	, VideoFrameDuration(FTimespan::Zero())
	, VideoOutputDim(FIntPoint::ZeroValue)
	, VideoPreviousTime(FTimespan::MinValue())
	, VideoSampleFormat(EMediaTextureSampleFormat::CharAYUV)
	, VideoSamplePool(new FVlcMediaTextureSamplePool)
{ }


FVlcMediaPlayerCallbacks::~FVlcMediaPlayerCallbacks()
{
	Shutdown();

	delete AudioSamplePool;
	AudioSamplePool = nullptr;

	delete Samples;
	Samples = nullptr;

	delete VideoSamplePool;
	VideoSamplePool = nullptr;
}


/* FVlcMediaOutput interface
 *****************************************************************************/

IMediaSamples& FVlcMediaPlayerCallbacks::GetSamples()
{
	return *Samples;
}


void FVlcMediaPlayerCallbacks::Initialize(libvlc_media_player_t& InPlayer)
{
	Shutdown();

	Player = &InPlayer;

	// register callbacks
	libvlc_audio_set_format_callbacks(
		Player,
		&FVlcMediaPlayerCallbacks::StaticAudioSetupCallback,
		&FVlcMediaPlayerCallbacks::StaticAudioCleanupCallback
	);

	libvlc_audio_set_callbacks(
		Player,
		&FVlcMediaPlayerCallbacks::StaticAudioPlayCallback,
		&FVlcMediaPlayerCallbacks::StaticAudioPauseCallback,
		&FVlcMediaPlayerCallbacks::StaticAudioResumeCallback,
		&FVlcMediaPlayerCallbacks::StaticAudioFlushCallback,
		&FVlcMediaPlayerCallbacks::StaticAudioDrainCallback,
		this
	);

	libvlc_video_set_format_callbacks(
		Player,
		&FVlcMediaPlayerCallbacks::StaticVideoSetupCallback,
		&FVlcMediaPlayerCallbacks::StaticVideoCleanupCallback
	);

	libvlc_video_set_callbacks(
		Player,
		&FVlcMediaPlayerCallbacks::StaticVideoLockCallback,
		&FVlcMediaPlayerCallbacks::StaticVideoUnlockCallback,
		&FVlcMediaPlayerCallbacks::StaticVideoDisplayCallback,
		this
	);
}


void FVlcMediaPlayerCallbacks::Shutdown()
{
	if (Player == nullptr)
	{
		return;
	}

	// unregister callbacks
	libvlc_audio_set_callbacks(Player, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
	libvlc_audio_set_format_callbacks(Player, nullptr, nullptr);

	libvlc_video_set_callbacks(Player, nullptr, nullptr, nullptr, nullptr);
	libvlc_video_set_format_callbacks(Player, nullptr, nullptr);

	AudioSamplePool->Reset();
	VideoSamplePool->Reset();

	CurrentTime = FTimespan::Zero();

	Player = nullptr;
}


/* FVlcMediaOutput static functions
*****************************************************************************/

void FVlcMediaPlayerCallbacks::StaticAudioCleanupCallback(void* Opaque)
{
	UE_LOG(LogVlcMediaPlayer, VeryVerbose, TEXT("Callbacks %llx: StaticAudioCleanupCallback"), Opaque);
}


void FVlcMediaPlayerCallbacks::StaticAudioDrainCallback(void* Opaque)
{
	UE_LOG(LogVlcMediaPlayer, VeryVerbose, TEXT("Callbacks %llx: StaticAudioDrainCallback"), Opaque);
}


void FVlcMediaPlayerCallbacks::StaticAudioFlushCallback(void* Opaque, int64 Timestamp)
{
	UE_LOG(LogVlcMediaPlayer, VeryVerbose, TEXT("Callbacks %llx: StaticAudioFlushCallback"), Opaque);
}


void FVlcMediaPlayerCallbacks::StaticAudioPauseCallback(void* Opaque, int64 Timestamp)
{
	UE_LOG(LogVlcMediaPlayer, VeryVerbose, TEXT("Callbacks %llx: StaticAudioPauseCallback (Timestamp = %i)"), Opaque, Timestamp);

	// do nothing; pausing is handled in Update
}


void FVlcMediaPlayerCallbacks::StaticAudioPlayCallback(void* Opaque, const void* Samples, uint32 Count, int64 Timestamp)
{
	auto Callbacks = (FVlcMediaPlayerCallbacks*)Opaque;

	if (Callbacks == nullptr)
	{
		return;
	}

	UE_LOG(LogVlcMediaPlayer, VeryVerbose, TEXT("Callbacks %llx: StaticAudioPlayCallback (Count = %i, Timestamp = %i, Queue = %i)"),
		Opaque,
		Count,
		Timestamp,
		Callbacks->Samples->NumAudio()
	);

	// create & add sample to queue
	auto AudioSample = Callbacks->AudioSamplePool->AcquireShared();

	const FTimespan Delay = FTimespan::FromMicroseconds(Timestamp - clock());
	const FTimespan Duration = FTimespan::FromMicroseconds((Count * 1000000) / Callbacks->AudioSampleRate);
	const SIZE_T SamplesSize = Count * Callbacks->AudioSampleSize * Callbacks->AudioChannels;

	if (AudioSample->Initialize(
		Samples,
		SamplesSize,
		Count,
		Callbacks->AudioChannels,
		Callbacks->AudioSampleFormat,
		Callbacks->AudioSampleRate,
		Callbacks->CurrentTime + Delay,
		Duration))
	{
		Callbacks->Samples->AddAudio(AudioSample);
	}
}


void FVlcMediaPlayerCallbacks::StaticAudioResumeCallback(void* Opaque, int64 Timestamp)
{
	UE_LOG(LogVlcMediaPlayer, VeryVerbose, TEXT("Callbacks %llx: StaticAudioResumeCallback (Timestamp = %i)"), Opaque, Timestamp);

	// do nothing; resuming is handled in Update
}


int FVlcMediaPlayerCallbacks::StaticAudioSetupCallback(void** Opaque, ANSICHAR* Format, uint32* Rate, uint32* Channels)
{
	auto Callbacks = *(FVlcMediaPlayerCallbacks**)Opaque;

	if (Callbacks == nullptr)
	{
		return -1;
	}

	UE_LOG(LogVlcMediaPlayer, VeryVerbose, TEXT("Callbacks %llx: StaticAudioSetupCallback (Format = %s, Rate = %i, Channels = %i)"),
		Opaque,
		ANSI_TO_TCHAR(Format),
		*Rate,
		*Channels
	);

	// setup audio format
	if (*Channels > 8)
	{
		*Channels = 8;
	}

	if (FMemory::Memcmp(Format, "S8  ", 4) == 0)
	{
		Callbacks->AudioSampleFormat = EMediaAudioSampleFormat::Int8;
		Callbacks->AudioSampleSize = 1;
	}
	else if (FMemory::Memcmp(Format, "S16N", 4) == 0)
	{
		Callbacks->AudioSampleFormat = EMediaAudioSampleFormat::Int16;
		Callbacks->AudioSampleSize = 2;
	}
	else if (FMemory::Memcmp(Format, "S32N", 4) == 0)
	{
		Callbacks->AudioSampleFormat = EMediaAudioSampleFormat::Int32;
		Callbacks->AudioSampleSize = 4;
	}
	else if (FMemory::Memcmp(Format, "FL32", 4) == 0)
	{
		Callbacks->AudioSampleFormat = EMediaAudioSampleFormat::Float;
		Callbacks->AudioSampleSize = 4;
	}
	else if (FMemory::Memcmp(Format, "FL64", 4) == 0)
	{
		Callbacks->AudioSampleFormat = EMediaAudioSampleFormat::Double;
		Callbacks->AudioSampleSize = 8;
	}
	else if (FMemory::Memcmp(Format, "U8  ", 4) == 0)
	{
		// unsigned integer fall back
		FMemory::Memcpy(Format, "S8  ", 4);
		Callbacks->AudioSampleFormat = EMediaAudioSampleFormat::Int8;
		Callbacks->AudioSampleSize = 1;
	}
	else
	{
		// unsupported format fall back
		FMemory::Memcpy(Format, "S16N", 4);
		Callbacks->AudioSampleFormat = EMediaAudioSampleFormat::Int16;
		Callbacks->AudioSampleSize = 2;
	}

	Callbacks->AudioChannels = *Channels;
	Callbacks->AudioSampleRate = *Rate;

	return 0;
}


void FVlcMediaPlayerCallbacks::StaticVideoCleanupCallback(void *Opaque)
{
	// do nothing
}


void FVlcMediaPlayerCallbacks::StaticVideoDisplayCallback(void* Opaque, void* Picture)
{
	auto Callbacks = (FVlcMediaPlayerCallbacks*)Opaque;
	auto VideoSample = (FVlcMediaPlayerTextureSample*)Picture;

	if ((Callbacks == nullptr) || (VideoSample == nullptr))
	{
		return;
	}

	UE_LOG(LogVlcMediaPlayer, VeryVerbose, TEXT("Callbacks %llx: StaticVideoDisplayCallback (CurrentTime = %s, Queue = %i)"),
		Opaque, *Callbacks->CurrentTime.ToString(),
		Callbacks->Samples->NumVideoSamples()
	);

	VideoSample->SetTime(Callbacks->CurrentTime);

	// add sample to queue
	Callbacks->Samples->AddVideo(Callbacks->VideoSamplePool->ToShared(VideoSample));
}


void* FVlcMediaPlayerCallbacks::StaticVideoLockCallback(void* Opaque, void** Planes)
{
	auto Callbacks = (FVlcMediaPlayerCallbacks*)Opaque;
	check(Callbacks != nullptr);

	FMemory::Memzero(Planes, 5 * sizeof(void*));

	// skip if already processed
	if (Callbacks->VideoPreviousTime == Callbacks->CurrentTime)
	{
		// VLC currently requires a valid buffer or it will crash
		Planes[0] = FMemory::Malloc(Callbacks->VideoBufferStride * Callbacks->VideoBufferDim.Y, 32);
		return nullptr;
	}

	UE_LOG(LogVlcMediaPlayer, VeryVerbose, TEXT("Callbacks %llx: StaticVideoLockCallback (CurrentTime = %s)"),
		Opaque,
		*Callbacks->CurrentTime.ToString()
	);

	// create & initialize video sample
	auto VideoSample = Callbacks->VideoSamplePool->Acquire();

	if (VideoSample == nullptr)
	{
		// VLC currently requires a valid buffer or it will crash
		Planes[0] = FMemory::Malloc(Callbacks->VideoBufferStride * Callbacks->VideoBufferDim.Y, 32);
		return nullptr;
	}

	if (!VideoSample->Initialize(
		Callbacks->VideoBufferDim,
		Callbacks->VideoOutputDim,
		Callbacks->VideoSampleFormat,
		Callbacks->VideoBufferStride,
		Callbacks->VideoFrameDuration))
	{
		// VLC currently requires a valid buffer or it will crash
		Planes[0] = FMemory::Malloc(Callbacks->VideoBufferStride * Callbacks->VideoBufferDim.Y, 32);
		return nullptr;
	}

	Callbacks->VideoPreviousTime = Callbacks->CurrentTime;
	Planes[0] = VideoSample->GetMutableBuffer();

	return VideoSample; // passed as Picture into unlock & display callbacks

}


unsigned FVlcMediaPlayerCallbacks::StaticVideoSetupCallback(void** Opaque, char* Chroma, unsigned* Width, unsigned* Height, unsigned* Pitches, unsigned* Lines)
{
	auto Callbacks = *(FVlcMediaPlayerCallbacks**)Opaque;
	
	if (Callbacks == nullptr)
	{
		return 0;
	}

	UE_LOG(LogVlcMediaPlayer, VeryVerbose, TEXT("Callbacks %llx: StaticVideoSetupCallback (Chroma = %s, Dim = %ix%i)"),
		Opaque,
		ANSI_TO_TCHAR(Chroma),
		*Width,
		*Height
	);

	// get video output size
	if (libvlc_video_get_size(Callbacks->Player, 0, (uint32*)&Callbacks->VideoOutputDim.X, (uint32*)&Callbacks->VideoOutputDim.Y) != 0)
	{
		Callbacks->VideoBufferDim = FIntPoint::ZeroValue;
		Callbacks->VideoOutputDim = FIntPoint::ZeroValue;
		Callbacks->VideoBufferStride = 0;

		return 0;
	}

	if (Callbacks->VideoOutputDim.GetMin() <= 0)
	{
		return 0;
	}

	// determine decoder & sample formats
	Callbacks->VideoBufferDim = FIntPoint(*Width, *Height);

	if (FCStringAnsi::Stricmp(Chroma, "AYUV") == 0)
	{
		Callbacks->VideoSampleFormat = EMediaTextureSampleFormat::CharAYUV;
		Callbacks->VideoBufferStride = *Width * 4;
	}
	else if (FCStringAnsi::Stricmp(Chroma, "RV32") == 0)
	{
		Callbacks->VideoSampleFormat = EMediaTextureSampleFormat::CharBGRA;
		Callbacks->VideoBufferStride = *Width * 4;
	}
	else if ((FCStringAnsi::Stricmp(Chroma, "UYVY") == 0) ||
		(FCStringAnsi::Stricmp(Chroma, "Y422") == 0) ||
		(FCStringAnsi::Stricmp(Chroma, "UYNV") == 0) ||
		(FCStringAnsi::Stricmp(Chroma, "HDYC") == 0))
	{
		Callbacks->VideoSampleFormat = EMediaTextureSampleFormat::CharUYVY;
		Callbacks->VideoBufferStride = *Width * 2;
	}
	else if ((FCStringAnsi::Stricmp(Chroma, "YUY2") == 0) ||
		(FCStringAnsi::Stricmp(Chroma, "V422") == 0) ||
		(FCStringAnsi::Stricmp(Chroma, "YUYV") == 0))
	{
		Callbacks->VideoSampleFormat = EMediaTextureSampleFormat::CharYUY2;
		Callbacks->VideoBufferStride = *Width * 2;
	}
	else if (FCStringAnsi::Stricmp(Chroma, "YVYU") == 0)
	{
		Callbacks->VideoSampleFormat = EMediaTextureSampleFormat::CharYVYU;
		Callbacks->VideoBufferStride = *Width * 2;
	}
	else
	{
		// reconfigure output for natively supported format
		const vlc_chroma_description_t* ChromaDescr = vlc_fourcc_GetChromaDescription(*(vlc_fourcc_t*)Chroma);

		if (ChromaDescr->plane_count == 0)
		{
			return 0;
		}

		if (ChromaDescr->plane_count > 1)
		{
			FMemory::Memcpy(Chroma, "YUY2", 4);

			Callbacks->VideoBufferDim = FIntPoint(Align(Callbacks->VideoOutputDim.X, 16) / 2, Align(Callbacks->VideoOutputDim.Y, 16));
			Callbacks->VideoSampleFormat = EMediaTextureSampleFormat::CharYUY2;
			Callbacks->VideoBufferStride = Callbacks->VideoBufferDim.X * 4;
			*Height = Callbacks->VideoBufferDim.Y;
		}
		else
		{
			FMemory::Memcpy(Chroma, "RV32", 4);

			Callbacks->VideoBufferDim = Callbacks->VideoOutputDim;
			Callbacks->VideoSampleFormat = EMediaTextureSampleFormat::CharBGRA;
			Callbacks->VideoBufferStride = Callbacks->VideoBufferDim.X * 4;
		}
	}

	// get other video properties
	//Callbacks->VideoFrameDuration = FTimespan::FromSeconds(1.0 / FVlc::MediaPlayerGetFps(Callbacks->Player));

	Callbacks->VideoFrameDuration = FTimespan::FromMilliseconds(1);

	// initialize decoder
	Lines[0] = Callbacks->VideoBufferDim.Y;
	Pitches[0] = Callbacks->VideoBufferStride;

	return 1;
}


void FVlcMediaPlayerCallbacks::StaticVideoUnlockCallback(void* Opaque, void* Picture, void* const* Planes)
{
	if ((Opaque != nullptr) && (Picture != nullptr))
	{
		UE_LOG(LogVlcMediaPlayer, VeryVerbose, TEXT("Callbacks %llx: StaticVideoUnlockCallback"), Opaque);
	}

	// discard temporary buffer for VLC crash workaround
	if ((Picture == nullptr) && (Planes != nullptr) && (Planes[0] != nullptr))
	{
		FMemory::Free(Planes[0]);
	}
}
