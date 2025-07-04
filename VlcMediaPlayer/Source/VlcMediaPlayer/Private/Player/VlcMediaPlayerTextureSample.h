// Copyright 2024-2025, obitodaitu. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "IMediaTextureSample.h"
#include "MediaObjectPool.h"
#include "Math/IntPoint.h"
#include "Misc/Timespan.h"
#include "Templates/SharedPointer.h"
#include <HAL/UnrealMemory.h>


/**
 * Texture sample generated by VlcMedia player.
 */
class FVlcMediaPlayerTextureSample
	: public IMediaTextureSample
	, public IMediaPoolable
{
public:

	/** Default constructor. */
	FVlcMediaPlayerTextureSample()
		: Buffer(nullptr)
		, BufferSize(0)
		, Dim(FIntPoint::ZeroValue)
		, Duration(FTimespan::Zero())
		, OutputDim(FIntPoint::ZeroValue)
		, SampleFormat(EMediaTextureSampleFormat::Undefined)
		, Stride(0)
		, Time(FTimespan::Zero())
	{ }

	/** Virtual destructor. */
	virtual ~FVlcMediaPlayerTextureSample()
	{
		FreeBuffer();
	}

public:

	/**
	 * Get a writable pointer to the sample buffer.
	 *
	 * @return The buffer.
	 * @see Initialize
	 */
	void* GetMutableBuffer()
	{
		return Buffer;
	}

	/**
	 * Initialize the sample.
	 *
	 * @param InDim The sample buffer's width and height (in pixels).
	 * @param InOutputDim The sample's output width and height (in pixels).
	 * @param InSampleFormat The sample format.
	 * @param InStride Number of bytes per pixel row.
	 * @param InDuration The duration for which the sample is valid.
	 * @return true on success, false otherwise.
	 */
	bool Initialize(
		const FIntPoint& InDim,
		const FIntPoint& InOutputDim,
		EMediaTextureSampleFormat InSampleFormat,
		uint32 InStride,
		FTimespan InDuration)
	{
		if (InSampleFormat == EMediaTextureSampleFormat::Undefined)
		{
			return false;
		}

		const SIZE_T RequiredBufferSize = InStride * InDim.Y;

		if (RequiredBufferSize == 0)
		{
			return false;
		}

		if (RequiredBufferSize > BufferSize)
		{
			Buffer = FMemory::Realloc(Buffer, RequiredBufferSize, 32);
			BufferSize = RequiredBufferSize;
		}

		Dim = InDim;
		Duration = InDuration;
		OutputDim = InOutputDim;
		SampleFormat = InSampleFormat;
		Stride = InStride;

		return true;
	}

	/**
	 * Set the time for which the sample was generated.
	 *
	 * @param InTime The time to set (in the player's clock).
	 */
	void SetTime(FTimespan InTime)
	{
		Time = InTime;
	}

public:

	//~ IMediaTextureSample interface

	virtual const void* GetBuffer() override
	{
		return Buffer;
	}

	virtual FIntPoint GetDim() const override
	{
		return Dim;
	}

	virtual FTimespan GetDuration() const override
	{
		return Duration;
	}

	virtual EMediaTextureSampleFormat GetFormat() const override
	{
		return SampleFormat;
	}

	virtual FIntPoint GetOutputDim() const override
	{
		return OutputDim;
	}

	virtual uint32 GetStride() const override
	{
		return Stride;
	}

#if WITH_ENGINE
	virtual FRHITexture* GetTexture() const override
	{
		return nullptr;
	}
#endif //WITH_ENGINE

	virtual FMediaTimeStamp GetTime() const override
	{
		return FMediaTimeStamp(Time);
	}

	virtual bool IsCacheable() const override
	{
		return true;
	}

	virtual bool IsOutputSrgb() const override
	{
		return true;
	}

protected:

	/** Free the sample buffer. */
	void FreeBuffer()
	{
		if (Buffer != nullptr)
		{
			FMemory::Free(Buffer);

			Buffer = nullptr;
			BufferSize = 0;
		}
	}

private:

	/** The sample's frame buffer. */
	void* Buffer;

	/** Allocated size of the buffer (in bytes). */
	SIZE_T BufferSize;

	/** Width and height of the texture sample. */
	FIntPoint Dim;

	/** Duration for which the sample is valid. */
	FTimespan Duration;

	/** Width and height of the output. */
	FIntPoint OutputDim;

	/** The sample format. */
	EMediaTextureSampleFormat SampleFormat;

	/** Number of bytes per pixel row. */
	uint32 Stride;

	/** Play time for which the sample was generated. */
	FTimespan Time;
};


/** Implements a pool for VLC texture sample objects. */
class FVlcMediaTextureSamplePool : public TMediaObjectPool<FVlcMediaPlayerTextureSample> { };
