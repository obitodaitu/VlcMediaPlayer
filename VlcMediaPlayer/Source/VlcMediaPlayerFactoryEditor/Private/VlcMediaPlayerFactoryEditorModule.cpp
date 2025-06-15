// Copyright 2024-2025, obitodaitu. All Rights Reserved.

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"


/**
 * Implements the VlcMediaEditor module.
 */
class FVlcMediaPlayerFactoryEditorModule
	: public IModuleInterface
{
public:

	//~ IModuleInterface interface

	virtual void StartupModule() override { }
	virtual void ShutdownModule() override { }
};


IMPLEMENT_MODULE(FVlcMediaPlayerFactoryEditorModule, VlcMediaPlayerFactoryEditor);
