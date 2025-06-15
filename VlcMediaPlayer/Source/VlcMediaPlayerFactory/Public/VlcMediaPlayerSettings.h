// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

#include "VlcMediaPlayerSettings.generated.h"


/**
 * Available levels for LibVLC log messages.
 */
UENUM()
enum class EVlcMediaPlayerLogLevel : uint8
{
	/** Error messages. */
	Error = 0,

	/** Warnings and potential errors. */
	Warning = 1,

	/** Debug messages. */
	Debug = 2,
};


/**
 * Settings for the VlcMedia plug-in.
 */
UCLASS(config=Engine)
class VLCMEDIAPLAYERFACTORY_API UVlcMediaPlayerSettings
	: public UObject
{
	GENERATED_BODY()

public:

	/** Default constructor. */
	UVlcMediaPlayerSettings();

public:

	/** Caching duration for optical media (default = 300 ms). */
	UPROPERTY(config, EditAnywhere, Category=Caching)
	FTimespan DiscCaching;

	/** Caching duration for local files (default = 300 ms). */
	UPROPERTY(config, EditAnywhere, Category=Caching)
	FTimespan FileCaching;

	/** Caching duration for cameras and microphones (default = 300 ms). */
	UPROPERTY(config, EditAnywhere, Category=Caching)
	FTimespan LiveCaching;

	/** Caching duration for network resources (default = 1000 ms). */
	UPROPERTY(config, EditAnywhere, Category=Caching)
	FTimespan NetworkCaching;
};
