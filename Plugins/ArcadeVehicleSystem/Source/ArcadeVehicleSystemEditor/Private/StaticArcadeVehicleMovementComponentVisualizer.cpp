/** Created and owned by Furious Production LTD @ 2023. **/

#include "StaticArcadeVehicleMovementComponentVisualizer.h"
#include "Movement/StaticArcadeVehicleMovementComponent.h"

#define SAFE_RETURN(Statement) \
if(!IsValid(Statement)) \
{ \
	return; \
}

void FStaticArcadeVehicleMovementComponentVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	/* Cache currently visualized component. */
	const UStaticArcadeVehicleMovementComponent* pVehicleMovement = Cast<UStaticArcadeVehicleMovementComponent>(Component);
	SAFE_RETURN(pVehicleMovement);
	const FVehicleSettings& settings = pVehicleMovement->GetVehicleSettings();

	/* Cache suspension parent bone transform, or if not valid, use component transform. */
	USceneComponent* pVehicleMesh = pVehicleMovement->GetOwner()->GetRootComponent();
	FTransform componentTransform = pVehicleMesh->GetComponentTransform();

	/* Calculate trace direction. */
	const FVector suspensionTraceDirectionBase = -componentTransform.GetRotation().GetUpVector();
	const FVector suspensionTraceDirection = suspensionTraceDirectionBase * settings.Suspension.TraceLength;

	/* Draw suspension raycasts. */
	for (const FVehicleSuspensionSpring& spring : settings.Suspension.Springs)
	{
		/* Convert suspension location from local to world space. */
		const FVector suspensionWorld = pVehicleMesh->GetSocketLocation(spring.BoneName);

		/* Calculate end location for the suspension ray. */
		const FVector suspensionRayEnd = suspensionWorld + suspensionTraceDirection;

		/* Draw trace line. */
		PDI->DrawLine(suspensionWorld, suspensionRayEnd, FLinearColor::Red, 0, 0.5f);

		/* With the right thickness, draw hit sphere. */
		if (settings.Suspension.TraceThickness > 0.f)
		{
			DrawWireSphere(PDI, suspensionRayEnd, FColor::Red, settings.Suspension.TraceThickness, 24, 0, 0.5f);
		}

		/* Visualize wheel swing. */
		const FVector springLocationDirection = (suspensionWorld - componentTransform.GetLocation()).GetSafeNormal();
		const float springDot = FVector::DotProduct(pVehicleMesh->GetRightVector(), springLocationDirection);
		const float directionMultiplierBase = FMath::Sign(springDot);
		const float directionMultiplier = 50.f * directionMultiplierBase;
		FVector direction = (pVehicleMesh->GetRightVector().Rotation() + FRotator(spring.SwingMinMax.X, 0.f, 0.f)).Vector();
		PDI->DrawLine(suspensionWorld, suspensionWorld + direction * directionMultiplier, FColor::Magenta, 0, 1.f);
		direction = (pVehicleMesh->GetRightVector().Rotation() + FRotator(spring.SwingMinMax.Y, 0.f, 0.f)).Vector();
		PDI->DrawLine(suspensionWorld, suspensionWorld + direction * directionMultiplier, FColor::Magenta, 0, 1.f);

		/* Visualize min/max wheel location. */
		const FVector2D minMaxOffset = spring.MinMaxOffsetZ;
		const FVector outDirection = pVehicleMesh->GetRightVector() * directionMultiplierBase;
		const FVector minLocation = suspensionWorld + suspensionTraceDirectionBase * -minMaxOffset.X;
		const FVector maxLocation = suspensionWorld + suspensionTraceDirectionBase * -minMaxOffset.Y;
		DrawWireCylinder(
			PDI,
			minLocation,
			suspensionTraceDirectionBase,
			FVector::CrossProduct(outDirection, suspensionTraceDirectionBase),
			outDirection,
			FLinearColor::Red,
			spring.WheelRadius,
			15.f,
			8,
			0,
			1.f);
		DrawWireCylinder(
			PDI,
			maxLocation,
			suspensionTraceDirectionBase,
			FVector::CrossProduct(outDirection, suspensionTraceDirectionBase),
			outDirection,
			FLinearColor::Green,
			spring.WheelRadius,
			15.f,
			8,
			0,
			1.f);	
	}
}
