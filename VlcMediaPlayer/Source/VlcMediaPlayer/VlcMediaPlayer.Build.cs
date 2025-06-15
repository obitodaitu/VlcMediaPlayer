// Copyright 2024-2025, obitodaitu. All Rights Reserved.

using System.IO;

namespace UnrealBuildTool.Rules
{
	using System.IO;


	public class VlcMediaPlayer : ModuleRules
	{
		public VlcMediaPlayer(ReadOnlyTargetRules Target) : base(Target)
		{
			PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

			DynamicallyLoadedModuleNames.AddRange(
				new string[] {
					"Media",
				});

			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"Core",
					"CoreUObject",
					"MediaUtils",
					"Projects",
					"RenderCore",
					"VlcMediaPlayerFactory",
				});

			PrivateIncludePathModuleNames.AddRange(
				new string[] {
					"Media",
				});

			PrivateIncludePaths.AddRange(
				new string[] {
                    "VlcMediaPlayer/Private",
                    "VlcMediaPlayer/Private/Player",
                });


            // add VLC libraries
            string BaseDirectory = Path.GetFullPath(Path.Combine(ModuleDirectory, ".."));
            string VlcDirectory = Path.Combine(BaseDirectory, "ThirdParty", "vlc", Target.Platform.ToString());

			//if (Target.Platform == UnrealTargetPlatform.Linux)
			//{
			//	VlcDirectory = Path.Combine(VlcDirectory, "lib");
			//	RuntimeDependencies.Add(Path.Combine(VlcDirectory, "libvlc.so"));
			//	RuntimeDependencies.Add(Path.Combine(VlcDirectory, "libvlc.so.5"));
			//	RuntimeDependencies.Add(Path.Combine(VlcDirectory, "libvlc.so.5.6.0"));
			//	RuntimeDependencies.Add(Path.Combine(VlcDirectory, "libvlccore.so"));
			//	RuntimeDependencies.Add(Path.Combine(VlcDirectory, "libvlccore.so.9"));
			//	RuntimeDependencies.Add(Path.Combine(VlcDirectory, "libvlccore.so.9.0.0"));
			//}
			//else if (Target.Platform == UnrealTargetPlatform.Mac)
			//{
			//	RuntimeDependencies.Add(Path.Combine(VlcDirectory, "libvlc.dylib"));
			//	RuntimeDependencies.Add(Path.Combine(VlcDirectory, "libvlc.5.dylib"));
			//	RuntimeDependencies.Add(Path.Combine(VlcDirectory, "libvlccore.dylib"));
			//	RuntimeDependencies.Add(Path.Combine(VlcDirectory, "libvlccore.9.dylib"));
			//}

			if (Target.Platform == UnrealTargetPlatform.Win64)
			{

                PublicSystemLibraries.AddRange(new string[] {
                    "ws2_32.lib",
                });

                PublicIncludePaths.Add(Path.Combine(VlcDirectory, "sdk", "include"));
                PublicIncludePaths.Add(Path.Combine(VlcDirectory, "sdk", "include", "vlc", "plugins"));
               
                PublicAdditionalLibraries.Add(Path.Combine(VlcDirectory, "sdk", "lib", "libvlc.lib"));
                PublicAdditionalLibraries.Add(Path.Combine(VlcDirectory, "sdk", "lib", "libvlccore.lib"));


                PublicDelayLoadDLLs.Add("libvlc.dll");
                PublicDelayLoadDLLs.Add("libvlccore.dll");

                RuntimeDependencies.Add(Path.Combine("$(ProjectDir)", "Binaries", "Win64", "libvlc.dll"), Path.Combine(VlcDirectory, "libvlc.dll"));
                RuntimeDependencies.Add(Path.Combine("$(ProjectDir)", "Binaries", "Win64", "libvlccore.dll"), Path.Combine(VlcDirectory, "libvlccore.dll"));
            }

			// add VLC plugins
			string PluginDirectory = Path.Combine(VlcDirectory, "plugins");
            
			if (Target.Platform == UnrealTargetPlatform.Linux)
			{
				PluginDirectory = Path.Combine(VlcDirectory, "vlc", "plugins");
			}

            if (Directory.Exists(PluginDirectory))
			{
				foreach (string Plugin in Directory.EnumerateFiles(PluginDirectory, "*.*", SearchOption.AllDirectories))
				{
                    RuntimeDependencies.Add(Path.Combine(PluginDirectory, Plugin));
                }
			}
		}
    } 
}
