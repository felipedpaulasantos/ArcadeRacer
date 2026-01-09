/** Created and owned by Furious Production LTD @ 2023. **/

using UnrealBuildTool;

public class ArcadeVehicleSystem : ModuleRules
{
	public ArcadeVehicleSystem(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.Add("Core");
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"NavigationSystem",
				"AIModule",
				"CinematicCamera"
			}
			);
		
#if UE_5_6_OR_LATER
		PublicDefinitions.Add("UE_5_6_OR_LATER=1");
#else
		PublicDefinitions.Add("UE_5_6_OR_LATER=0");
#endif
	}
}
