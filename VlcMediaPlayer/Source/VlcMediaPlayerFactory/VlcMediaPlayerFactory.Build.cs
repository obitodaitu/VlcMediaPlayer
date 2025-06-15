// Copyright 2024-2025, obitodaitu. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class VlcMediaPlayerFactory : ModuleRules
	{
		public VlcMediaPlayerFactory(ReadOnlyTargetRules Target) : base(Target)
		{
			PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

			DynamicallyLoadedModuleNames.AddRange(
				new string[] {
					"Media",
				});

			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"MediaAssets",
				});

			PrivateIncludePathModuleNames.AddRange(
				new string[] {
					"Media",
					"VlcMediaPlayer",
				});

			PrivateIncludePaths.AddRange(
				new string[] {
					"VlcMediaPlayerFactory/Private",
				});

			PublicDependencyModuleNames.AddRange(
				new string[] {
					"Core",
					"CoreUObject",
				});

			if (Target.Type == TargetRules.TargetType.Editor)
			{
				DynamicallyLoadedModuleNames.Add("Settings");
				PrivateIncludePathModuleNames.Add("Settings");
			}

			if ((Target.Platform == UnrealTargetPlatform.Mac) ||
				(Target.Platform == UnrealTargetPlatform.Linux) ||
				(Target.Platform == UnrealTargetPlatform.Win64))
			{
				DynamicallyLoadedModuleNames.Add("VlcMediaPlayer");
			}
		}
	}
}
