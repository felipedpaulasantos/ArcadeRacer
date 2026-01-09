/** Created and owned by Furious Production LTD @ 2023. **/

#include "Animations/TankVehicleAnimationInstance.h"

#include "Movement/ArcadeVehicleMovementComponent.h"

UTankVehicleAnimationInstance::UTankVehicleAnimationInstance()
{
	InPlaceWheelRotationMultiplier = 1.f;
	WheelSideRotationMultiplier = 0.5f;
	VehicleSpeedForInverseWheelRotation = 5.f;
}

void UTankVehicleAnimationInstance::CalculateWheelsRotation(float DeltaTime)
{
	/* Rotation multiplier based on the vehicle direction. */
	const float rotationDirection = m_pVehicleMovementComponent->IsMovingBackward() ? 1.f : -1.f;

	/* Cache vehicle vehicleSettings. */
	const FVehicleSettings& vehicleSettings = m_pVehicleMovementComponent->GetVehicleSettings();

	/* Allocate wheel rotations if they are not present. */
	AllocateWheels(vehicleSettings.Suspension);

	/* Check if wheels event should ever rotate. */
	const bool bShouldRotateWheels = !Settings.Wheels.bStopRotationOnBraking || !m_pVehicleMovementComponent->IsBraking() || m_pVehicleMovementComponent->IsEngineBraking();

	/* Rotate wheels if allowed. */
	if (bShouldRotateWheels)
	{
		/* Calculate speed absolute. */
		const float currentSpeedAbsolute = FMath::Abs(m_pVehicleMovementComponent->GetCurrentSpeed());

		/* Cache vehicle turning input. */
		const float turningInput = m_pVehicleMovementComponent->GetTurningInput();

		/* Iterate over all springs. */
		int32 springIndex = INDEX_NONE;
		for (const FVehicleSuspensionSpring& spring : vehicleSettings.Suspension.Springs)
		{
			/* Bump up spring index. */
			springIndex++;

			/* Cache iterated wheel. */
			FVehicleWheelAnimationInfo& wheel = Settings.Wheels.Registry[springIndex];

			/* This spring wheel must be on one of the vehicle sides. */
			if (wheel.Side == VehicleWheelSide::None)
			{
				continue;
			}

			/* If the speed is zero. */
			if (FMath::IsNearlyZero(m_pVehicleMovementComponent->GetCurrentSpeed(), VehicleSpeedForInverseWheelRotation))
			{
				/* If turning in any direction. */
				if(turningInput != 0.f)
				{
					/* Calculate wheel rotation offset. */
					const float wheelRotationDirection = (wheel.Side == VehicleWheelSide::Left && turningInput > 0.f) || (wheel.Side == VehicleWheelSide::Right && turningInput < 0.f) ? -1.f : 1.f;

					/* In this case, the turning speed defines the wheel rotation speed. */
					const float wheelPerimeter = 2.f * PI * spring.TargetHeight * 0.01f;
					const float travelDistance = FMath::Abs(turningInput) * 1000.f / 3600.f * DeltaTime;
					wheel.Rotation += ((travelDistance / wheelPerimeter) * 360.f) * wheelRotationDirection * InPlaceWheelRotationMultiplier;
				}
			}
			/* If the speed is not zero. */
			else
			{
				/* Calculate turning alpha from the wheel direction. */
				const float wheelsDirection = Settings.Wheels.CurrentDirection;
				const float turningAlpha = FMath::Max((1.f - FMath::Clamp(FMath::Abs(wheelsDirection) / Settings.Wheels.MaxDirection, 0.f, 1.f)), 0.01f);

				/* If wheels direction is zero, we are handling regular rotation. */
				if (wheelsDirection == 0.f)
				{
					/* Calculate wheel perimeter based on the suspension height for each wheel. */
					const float wheelPerimeter = 2.f * PI * spring.TargetHeight * 0.01f;

					/* Calculate travel distance of the vehicle. */
					const float travelDistance = currentSpeedAbsolute * 1000.f / 3600.f * DeltaTime;

					/* Based on the travel distance we can finally calculate wheel rotation. */
					wheel.Rotation += ((travelDistance / wheelPerimeter) * 360.f) * rotationDirection;
				}
				/* Otherwise, slow down one side. */
				else
				{
					/* Slow down opposide side to the turn. */
					const bool bShouldSlowDown = (wheel.Side == VehicleWheelSide::Left && wheelsDirection < 0.f) || (wheel.Side == VehicleWheelSide::Right && wheelsDirection > 0.f);
					float speedMultiplier = 1.f;
					if (bShouldSlowDown)
					{
						speedMultiplier = FMath::Clamp(turningAlpha * WheelSideRotationMultiplier, 0.01f, 1.f);
					}

					/* Calculate regular rotation with speed multiplier. */
					const float wheelPerimeter = 2.f * PI * spring.TargetHeight * 0.01f;
					const float travelDistance = currentSpeedAbsolute * 1000.f / 3600.f * DeltaTime;
					wheel.Rotation += ((travelDistance / wheelPerimeter) * 360.f) * rotationDirection * speedMultiplier;
				}
			}
		}
	}
}
