/** Created and owned by Furious Production LTD @ 2023. **/

#include "Movement/ArcadeVehiclePathFollowingComponent.h"

bool UArcadeVehiclePathFollowingComponent::HasReachedDestination(const FVector& CurrentLocation) const
{
	// get cylinder at goal location
	FVector GoalLocation = *Path->GetPathPointLocation(Path->GetPathPoints().Num() - 1);
	float GoalRadius = 0.0f;
	float GoalHalfHeight = 0.0f;

	// take goal's current location, unless path is partial or last segment doesn't reach goal actor (used by tethered AI)
	if (DestinationActor.IsValid() && !Path->IsPartial() && bMoveToGoalOnLastSegment)
	{
		if (DestinationAgent)
		{
			const AActor* OwnerActor = GetOwner();
			FVector GoalOffset;
			DestinationAgent->GetMoveGoalReachTest(OwnerActor, MoveOffset, GoalOffset, GoalRadius, GoalHalfHeight);

			GoalLocation = FQuatRotationTranslationMatrix(DestinationActor->GetActorQuat(), DestinationAgent->GetNavAgentLocation()).TransformPosition(GoalOffset);
		}
		else
		{
			GoalLocation = DestinationActor->GetActorLocation();
		}
	}

	const float AcceptanceRangeToUse = GetFinalAcceptanceRadius(*Path, OriginalMoveRequestGoalLocation);
	return HasVehicleReachedDestination(GoalLocation, bReachTestIncludesGoalRadius ? GoalRadius : 0.0f, GoalHalfHeight, CurrentLocation
		, AcceptanceRangeToUse, bReachTestIncludesAgentRadius ? MinAgentRadiusPct : 0.0f);
}

bool UArcadeVehiclePathFollowingComponent::HasVehicleReachedDestination(const FVector& GoalLocation, float GoalRadius, float GoalHalfHeight, const FVector& AgentLocation, float RadiusThreshold, float AgentRadiusMultiplier) const
{
#if UE_5_6_OR_LATER
	if (NavMovementInterface == nullptr)
#else
	if (MovementComp == nullptr)
#endif
	{
		return false;
	}

	// get cylinder of moving agent
	float AgentRadius = 0.0f;
	float AgentHalfHeight = 0.0f;
#if UE_5_6_OR_LATER
	AActor* MovingAgent = Cast<AActor>(NavMovementInterface->GetOwnerAsObject());
#else
	AActor* MovingAgent = MovementComp->GetOwner();
#endif
	MovingAgent->GetSimpleCollisionCylinder(AgentRadius, AgentHalfHeight);

	// check if they overlap (with added AcceptanceRadius)
	const FVector ToGoal = GoalLocation - AgentLocation;

	const float Dist2DSq = ToGoal.SizeSquared2D();
	const float UseRadius = RadiusThreshold + GoalRadius + (AgentRadius * AgentRadiusMultiplier);
	if (Dist2DSq > FMath::Square(UseRadius))
	{
		return false;
	}

	return true;
}

