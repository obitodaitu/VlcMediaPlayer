// Copyright 2024-2025, obitodaitu. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class VlcMediaPlayerFactoryEditor : ModuleRules
	{
		public VlcMediaPlayerFactoryEditor(ReadOnlyTargetRules Target) : base(Target)
		{
			PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"Core",
					"CoreUObject",
					"MediaAssets",
					"UnrealEd",
				});

			PrivateIncludePaths.AddRange(
				new string[] {
                    "VlcMediaPlayerFactoryEditor/Private",
				});
		}
	}
}
