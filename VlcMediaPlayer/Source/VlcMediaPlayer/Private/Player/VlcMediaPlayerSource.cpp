// Copyright 2024-2025, obitodaitu. All Rights Reserved.

#include "VlcMediaPlayerSource.h"
#include "VlcMediaPlayerPrivate.h"




/* FVlcMediaReader structors
*****************************************************************************/

FVlcMediaPlayerSource::FVlcMediaPlayerSource(libvlc_instance_t* InVlcInstance)
	: Media(nullptr)
	, VlcInstance(InVlcInstance)
{ }


/* FVlcMediaReader interface
*****************************************************************************/

FTimespan FVlcMediaPlayerSource::GetDuration() const
{
	if (Media == nullptr)
	{
		return FTimespan::Zero();
	}

	int64 Duration = libvlc_media_get_duration(Media);

	if (Duration < 0)
	{
		return FTimespan::Zero();
	}

	return FTimespan(Duration * ETimespan::TicksPerMillisecond);
}


libvlc_media_t* FVlcMediaPlayerSource::OpenArchive(const TSharedRef<FArchive, ESPMode::ThreadSafe>& Archive, const FString& OriginalUrl)
{
	check(Media == nullptr);

	if (Archive->TotalSize() > 0)
	{
		Data = Archive;
		Media = libvlc_media_new_callbacks(
			VlcInstance,
			nullptr,
			&FVlcMediaPlayerSource::HandleMediaRead,
			&FVlcMediaPlayerSource::HandleMediaSeek,
			&FVlcMediaPlayerSource::HandleMediaClose,
			this
		);

		if (Media == nullptr)
		{
			UE_LOG(LogVlcMediaPlayer, Warning, TEXT("Failed to open media from archive: %s (%s)"), *OriginalUrl, ANSI_TO_TCHAR(libvlc_errmsg()));
			Data.Reset();
		}
		else
		{
			CurrentUrl = OriginalUrl;
		}
	}

	return Media;
}


libvlc_media_t* FVlcMediaPlayerSource::OpenUrl(const FString& Url)
{
	check(Media == nullptr);

	Media = libvlc_media_new_location(VlcInstance, TCHAR_TO_ANSI(*Url));

	if (Media == nullptr)
	{
		UE_LOG(LogVlcMediaPlayer, Warning, TEXT("Failed to open media from URL: %s (%s)"), *Url, ANSI_TO_TCHAR(libvlc_errmsg()));
	}
	else
	{
		CurrentUrl = Url;
	}

	return Media;
}


void FVlcMediaPlayerSource::Close()
{
	if (Media != nullptr)
	{
		libvlc_media_release(Media);
		Media = nullptr;
	}

	Data.Reset();
	CurrentUrl.Reset();
}


/* FVlcMediaReader static functions
*****************************************************************************/

int FVlcMediaPlayerSource::HandleMediaOpen(void* Opaque, void** OutData, uint64* OutSize)
{
	auto Reader = (FVlcMediaPlayerSource*)Opaque;

	if ((Reader == nullptr) || !Reader->Data.IsValid())
	{
		return 0;
	}

	*OutSize = Reader->Data->TotalSize();

	return 0;
}


SSIZE_T FVlcMediaPlayerSource::HandleMediaRead(void* Opaque, unsigned char* Buffer, SIZE_T Length)
{
	auto Reader = (FVlcMediaPlayerSource*)Opaque;

	if (Reader == nullptr)
	{
		return -1;
	}

	TSharedPtr<FArchive, ESPMode::ThreadSafe> Data = Reader->Data;

	if (!Reader->Data.IsValid())
	{
		return -1;
	}

	SIZE_T DataSize = (SIZE_T)Data->TotalSize();
	SIZE_T BytesToRead = FMath::Min(Length, DataSize);
	SIZE_T DataPosition = Reader->Data->Tell();

	if ((DataSize - BytesToRead) < DataPosition)
	{
		BytesToRead = DataSize - DataPosition;
	}

	if (BytesToRead > 0)
	{
		Data->Serialize(Buffer, BytesToRead);
	}

	return (SSIZE_T)BytesToRead;
}


int FVlcMediaPlayerSource::HandleMediaSeek(void* Opaque, uint64 Offset)
{
	auto Reader = (FVlcMediaPlayerSource*)Opaque;

	if (Reader == nullptr)
	{
		return -1;
	}

	TSharedPtr<FArchive, ESPMode::ThreadSafe> Data = Reader->Data;

	if (!Reader->Data.IsValid())
	{
		return -1;
	}

	if ((uint64)Data->TotalSize() <= Offset)
	{
		return -1;
	}

	Reader->Data->Seek(Offset);

	return 0;
}


void FVlcMediaPlayerSource::HandleMediaClose(void* Opaque)
{
	auto Reader = (FVlcMediaPlayerSource*)Opaque;

	if (Reader != nullptr)
	{
		Reader->Data->Seek(0);
	}
}
