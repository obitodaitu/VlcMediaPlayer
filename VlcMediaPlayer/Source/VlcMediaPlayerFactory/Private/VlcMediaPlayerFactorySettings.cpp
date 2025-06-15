// Copyright 2024-2025, obitodaitu. All Rights Reserved.

#include "VlcMediaPlayerSettings.h"


UVlcMediaPlayerSettings::UVlcMediaPlayerSettings()
	: DiscCaching(FTimespan::FromMilliseconds(300.0))
	, FileCaching(FTimespan::FromMilliseconds(300.0))
	, LiveCaching(FTimespan::FromMilliseconds(300.0))
	, NetworkCaching(FTimespan::FromMilliseconds(1000.0))
{ }
