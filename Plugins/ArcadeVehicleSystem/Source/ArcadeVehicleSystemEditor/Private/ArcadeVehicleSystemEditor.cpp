/** Created and owned by Furious Production LTD @ 2023. **/

#include "ArcadeVehicleSystemEditor.h"
#include "SkeletalArcadeVehicleMovementComponentVisualizer.h"
#include "StaticArcadeVehicleMovementComponentVisualizer.h"
#include "Movement/ArcadeVehicleMovementComponent.h"
#include "Movement/StaticArcadeVehicleMovementComponent.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"

#define LOCTEXT_NAMESPACE "FArcadeVehicleSystemEditorModule"

void FArcadeVehicleSystemEditorModule::StartupModule()
{
	/* Registering vehicle movement component visualizer. */
	if (GUnrealEd)
	{
		GUnrealEd->RegisterComponentVisualizer(UArcadeVehicleMovementComponent::StaticClass()->GetFName(), MakeShareable(new FSkeletalArcadeVehicleMovementComponentVisualizer));
		GUnrealEd->RegisterComponentVisualizer(UStaticArcadeVehicleMovementComponent::StaticClass()->GetFName(), MakeShareable(new FStaticArcadeVehicleMovementComponentVisualizer));
	}
}

void FArcadeVehicleSystemEditorModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FArcadeVehicleSystemEditorModule, ArcadeVehicleSystemEditor)