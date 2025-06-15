// Copyright 2024-2025, obitodaitu. All Rights Reserved.

#include "IVlcMediaPlayerModule.h"
#include "VlcMediaPlayerPrivate.h"

#include "HAL/FileManager.h"
#include "Misc/OutputDeviceFile.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "UObject/Class.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/WeakObjectPtr.h"

#include "Interfaces/IPluginManager.h"
#include "VlcMediaPlayer.h"

#include "VlcWrapper.h"
#include <string>


DEFINE_LOG_CATEGORY(LogVlcMediaPlayer);

#define LOCTEXT_NAMESPACE "FVlcMediaPlayerModule"

typedef int(__cdecl* PFN_putenv)(const char*);

/**
 * Implements the VlcMedia module.
 */
class FVlcMediaPlayerModule
	: public IVlcMediaPlayerModule
{
public:

	/** Default constructor. */
	FVlcMediaPlayerModule()
	{ }

public:

	//~ IVlcMediaModule interface

	virtual TSharedPtr<IMediaPlayer, ESPMode::ThreadSafe> CreatePlayer(IMediaEventSink& EventSink) override
	{
		return MakeShared<FVlcMediaPlayer, ESPMode::ThreadSafe>(EventSink, VlcInstance);
	}

public:

	//~ IModuleInterface interface

	virtual void StartupModule() override
	{

		WSADATA wsaData;
		int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (result != 0)
		{
			return;
		}

		const FString BaseDir = IPluginManager::Get().FindPlugin(UE_PLUGIN_NAME)->GetBaseDir();
		const FString VlcDir = FPaths::Combine(*BaseDir, TEXT("Source"), TEXT("ThirdParty"), TEXT("vlc"));

#if PLATFORM_LINUX
		const FString LibDir = FPaths::Combine(*VlcDir, TEXT("Linux"), TEXT("x86_64-unknown-linux-gnu"), TEXT("lib"));
#elif PLATFORM_MAC
		const FString LibDir = FPaths::Combine(*VlcDir, TEXT("Mac"));
#elif PLATFORM_WINDOWS
		const FString LibDir = FPaths::Combine(*VlcDir, TEXT("Win64"));
#endif
		
		FString VlcPluginDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(*LibDir, TEXT("plugins")));

#if PLATFORM_LINUX
		VlcPluginDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(*LibDir, TEXT("vlc"), TEXT("plugins")));
#endif

		SetVLCPluginPath(VlcPluginDir);

		const auto Settings = GetDefault<UVlcMediaPlayerSettings>();

		// create LibVLC instance
		const ANSICHAR* Args[] =
		{
			// caching
			TCHAR_TO_ANSI(*(FString::Printf(TEXT("--disc-caching=%i"), (int32)Settings->DiscCaching.GetTotalMilliseconds()))),
			TCHAR_TO_ANSI(*(FString::Printf(TEXT("--file-caching=%i"), (int32)Settings->FileCaching.GetTotalMilliseconds()))),
			TCHAR_TO_ANSI(*(FString::Printf(TEXT("--live-caching=%i"), (int32)Settings->LiveCaching.GetTotalMilliseconds()))),
			TCHAR_TO_ANSI(*(FString::Printf(TEXT("--network-caching=%i"), (int32)Settings->NetworkCaching.GetTotalMilliseconds()))),

			"--vout=direct2d",

			// config
			"--ignore-config",

			// logging
#if UE_BUILD_DEBUG
			"--file-logging",
			TCHAR_TO_ANSI(*(FString(TEXT("--logfile=")) + LogFilePath)),
#endif

#if (UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT)
			"--verbose=2",
#else
			"--quiet",
#endif

			// output
			"--aout", "amem",
			"--intf", "dummy",
			"--text-renderer", "dummy",
			"--vout", "vmem",

			// performance
			"--drop-late-frames",

			// undesired features
			"--no-disable-screensaver",
			"--no-plugins-cache",
			"--no-snapshot-preview",
			"--no-video-title-show",

#if (UE_BUILD_SHIPPING || UE_BUILD_TEST)
			"--no-stats",
#endif

#if PLATFORM_LINUX
			"--no-xlib",
#endif
		};

		int Argc = sizeof(Args) / sizeof(*Args);
		VlcInstance = libvlc_new(Argc, Args);

		if (VlcInstance == nullptr)
		{
			UE_LOG(LogVlcMediaPlayer, Warning, TEXT("Failed to create VLC instance (%s)"), ANSI_TO_TCHAR(libvlc_errmsg()));
			return;
		}

	}

	virtual void ShutdownModule() override
	{
		// unregister logging callback
		libvlc_log_unset(VlcInstance);

		// release LibVLC instance
		libvlc_release(VlcInstance);
		VlcInstance = nullptr;

		WSACleanup();
	}

private:

	void SetVLCPluginPath(const FString& InPluginDir)
	{
		if (InPluginDir.IsEmpty())
			return;

		std::string AnsiPath = TCHAR_TO_UTF8(*InPluginDir);

		

#if PLATFORM_LINUX
		setenv("VLC_PLUGIN_PATH", AnsiPath.c_str(), true);
#elif PLATFORM_WINDOWS
		std::string EnvStr = "VLC_PLUGIN_PATH=" + AnsiPath;
		HMODULE hCRT = LoadLibraryW(L"msvcrt.dll");
		if (hCRT)
		{
#pragma warning(push)
#pragma warning(disable:4191)
			PFN_putenv pfn_putenv = (PFN_putenv)GetProcAddress(hCRT, "_putenv");
#pragma warning(pop)

			if (pfn_putenv)
			{
				pfn_putenv(EnvStr.c_str());
			}
			FreeLibrary(hCRT);
		}
#endif
		
	}


	

private:

	/** The LibVLC instance. */
	libvlc_instance_t* VlcInstance;
};

IMPLEMENT_MODULE(FVlcMediaPlayerModule, VlcMediaPlayer);

#undef LOCTEXT_NAMESPACE


