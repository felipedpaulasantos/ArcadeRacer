/** Created and owned by Furious Production LTD @ 2023. **/

#pragma once
#include "CoreMinimal.h"
#include "Navigation/PathFollowingComponent.h"
#include "ArcadeVehiclePathFollowingComponent.generated.h"

/**
	Path following component for the arcade vehicles.
	Implements simple path following fixes, so that creating
	AI movement is possible.
*/
UCLASS(BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class ARCADEVEHICLESYSTEM_API UArcadeVehiclePathFollowingComponent : public UPathFollowingComponent
{
	GENERATED_BODY()

public:
	/** UPathFollowingComponent interface. */
	bool HasReachedDestination(const FVector& CurrentLocation) const override;
	/** ~UPathFollowingComponent interface. */

	/** Function to determine whether this vehicle has reached its destination. */
	bool HasVehicleReachedDestination(const FVector& GoalLocation, float GoalRadius, float GoalHalfHeight, const FVector& AgentLocation, float RadiusThreshold, float AgentRadiusMultiplier) const;
};