/** Created and owned by Furious Production LTD @ 2023. **/

#include "Animations/ArcadeVehicleAnimationInstance.h"
#include "Movement/ArcadeVehicleMovementComponent.h"
#include "Engine/World.h"

UArcadeVehicleAnimationInstance::UArcadeVehicleAnimationInstance()
{
	m_bWasAccelerating = false;
	m_bWasBraking = false;
}

void UArcadeVehicleAnimationInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	/* Any vehicle-related calculations can only be performed outside the editor. */
	if (GetWorld()->WorldType != EWorldType::Game && GetWorld()->WorldType != EWorldType::PIE)
	{
		return;
	}

	/* Must have valid vehicle. */
	if (!IsValid(GetVehicleMovementComponent()))
	{
		return;
	}

	/* Perform wheels-related calculations. */
	CalculateWheelsDirection(DeltaSeconds);
	CalculateWheelsRotation(DeltaSeconds);
	CalculateWheelsOffsets(DeltaSeconds);

	/* Perform body-related calculations. */
	CalculateTilt(DeltaSeconds);
	CalculateRoll(DeltaSeconds);
}

UArcadeVehicleMovementComponent* UArcadeVehicleAnimationInstance::GetVehicleMovementComponent()
{
	if (!IsValid(m_pVehicleMovementComponent))
	{
		m_pVehicleMovementComponent = GetOwningActor()->FindComponentByClass<UArcadeVehicleMovementComponent>();
	}

	return m_pVehicleMovementComponent;
}

void UArcadeVehicleAnimationInstance::CalculateWheelsDirection(float DeltaTime)
{
	/* Calculate wheels direction using raw input. */
	const float turningInput = m_pVehicleMovementComponent->GetTurningInput();
	float currentWheelsDirection = turningInput * Settings.Wheels.MaxDirection;

	/* Check if wheels direction has changed - it most likely has. */
	if (currentWheelsDirection != Settings.Wheels.DirectionBeginEnd.Y)
	{
		/* Reset wheels timer. */
		Settings.Wheels.Timer = 0.f;

		/* Cache new begin and end values. */
		Settings.Wheels.DirectionBeginEnd.X = Settings.Wheels.CurrentDirection;
		Settings.Wheels.DirectionBeginEnd.Y = currentWheelsDirection;

		/* Calculate remaining duration for smoothing. */
		Settings.Wheels.CurrentDuration = Settings.Wheels.TransitionTime * (currentWheelsDirection - Settings.Wheels.CurrentDirection) / (Settings.Wheels.MaxDirection);
		Settings.Wheels.CurrentDuration = FMath::Abs(Settings.Wheels.CurrentDuration);
	}

	/* If the direction timer has still something to work out. */
	if (Settings.Wheels.Timer < Settings.Wheels.CurrentDuration)
	{
		/* Increase timer value. */
		Settings.Wheels.Timer += DeltaTime;

		/* Calculate what is current difference in value. */
		const float valueDifference = Settings.Wheels.DirectionBeginEnd.Y - Settings.Wheels.DirectionBeginEnd.X;

		/* Apply new wheels direction now. */
		Settings.Wheels.CurrentDirection = ((valueDifference * Settings.Wheels.Timer) / Settings.Wheels.CurrentDuration) + Settings.Wheels.DirectionBeginEnd.X;
	}
	/* If timer has concluded rotation. */
	else
	{
		/* Zero out timing values. */
		Settings.Wheels.Timer = 0.f;
		Settings.Wheels.CurrentDuration = 0.f;

		/* Snap final rotation hard. */
		Settings.Wheels.CurrentDirection = Settings.Wheels.DirectionBeginEnd.Y;
	}
}

void UArcadeVehicleAnimationInstance::CalculateWheelsRotation(float DeltaTime)
{
	/* Rotation multiplier based on the vehicle direction. */
	const float rotationDirection = m_pVehicleMovementComponent->IsMovingBackward() ? 1.f : -1.f;

	/* Cache vehicle vehicleSettings. */
	const FVehicleSettings& vehicleSettings = m_pVehicleMovementComponent->GetVehicleSettings();

	/* Allocate wheel rotations if they are not present. */
	AllocateWheels(vehicleSettings.Suspension);

	/* Check if wheels event should ever rotate. */
	const bool bShouldRotateWheels = !Settings.Wheels.bStopRotationOnBraking || !m_pVehicleMovementComponent->IsBraking() || m_pVehicleMovementComponent->IsEngineBraking();

	/* If vehicle is running - its speed isn't 0. */
	if (bShouldRotateWheels && m_pVehicleMovementComponent->GetCurrentSpeed() != 0.f)
	{
		/* Calculate speed absolute. */
		const float currentSpeedAbsolute = FMath::Abs(m_pVehicleMovementComponent->GetCurrentSpeed());
		
		/* Iterate over all springs. */
		int32 springIndex = INDEX_NONE;
		for (const FVehicleSuspensionSpring& spring : vehicleSettings.Suspension.Springs)
		{
			/* Bump up spring index. */
			springIndex++;

			/* Calculate wheel perimeter based on the suspension height for each wheel. */
			const float wheelPerimeter = 2.f * PI * spring.TargetHeight * 0.01f;

			/* Calculate travel distance of the vehicle. */
			const float travelDistance = currentSpeedAbsolute * 1000.f / 3600.f * DeltaTime;
			
			/* Based on the travel distance we can finally calculate wheel rotation. */
			Settings.Wheels.Registry[springIndex].Rotation += ((travelDistance / wheelPerimeter) * 360.f) * rotationDirection;
		}
	}
}

void UArcadeVehicleAnimationInstance::CalculateWheelsOffsets(float DeltaTime)
{
	/* Cache vehicle vehicleSettings. */
	const FVehicleSettings& vehicleSettings = m_pVehicleMovementComponent->GetVehicleSettings();

	/* Iterate over all wheels. */
	for (int32 i = 0; i < vehicleSettings.Suspension.Springs.Num(); ++i)
	{
		/* Fill wheels with spring data. */
		Settings.Wheels.Registry[i].Offset = vehicleSettings.Suspension.Springs[i].WheelOffset;
		Settings.Wheels.Registry[i].Swing = vehicleSettings.Suspension.Springs[i].CurrentSwing;
	}
}

void UArcadeVehicleAnimationInstance::CalculateTilt(float DeltaTime)
{
	/* Cache vehicle vehicleSettings. */
	const FVehicleSettings& vehicleSettings = m_pVehicleMovementComponent->GetVehicleSettings();

	/* Check current acceleration and braking flags of the vehicle. */
	const bool bIsAccelerating = m_pVehicleMovementComponent->IsAccelerating();
	const bool bIsBraking = m_pVehicleMovementComponent->IsBraking();

	/* Grab vehicle movement direction multiplier. */
	const float movementDirectionMultiplier = FMath::Sign(m_pVehicleMovementComponent->GetLastAppliedAcceleration());

	/* Cache last known acceleration absolute value. */
	const float accelerationAbsolute = FMath::Abs(m_pVehicleMovementComponent->GetLastAppliedAcceleration());

	/* Cache last known braking absolute value. */
	const float brakingAbsolute = FMath::Abs(m_pVehicleMovementComponent->GetLastAppliedBraking());

	/* Check if the tilt should be reset. That is when we start accelerating or start braking. */
	const bool bShouldResetTilt = (bIsAccelerating && !m_bWasAccelerating) || (bIsBraking && !m_bWasBraking);

	/* Assign new flag values, so we can check them the next frame for changes. */
	m_bWasAccelerating = bIsAccelerating;
	m_bWasBraking = bIsBraking;

	/* We will reset tilt only if it's required and it is quite low. */
	if (bShouldResetTilt && FMath::Abs(Settings.Tilt.CurrentTilt) < 0.1f)
	{
		/* Reset tilt timer so we start over. */
		Settings.Tilt.Timer = 0.f;

		/* Assign tilt runtime depending on the acceleration or braking. */
		Settings.Tilt.RuntimeTilt = bIsAccelerating ? Settings.Tilt.AccelerationTilt : Settings.Tilt.BrakingTilt;

		/* Acceleration tilt is stronger with lower speeds, but the braking tilt is stronger when speeds are higher. */
		/* Correct tilt speed value by calculating speed ratio. */
		float speedRatio = FMath::Clamp(FMath::Abs(m_pVehicleMovementComponent->GetCurrentSpeed() / Settings.Tilt.RuntimeTilt.TiltDampingSpeed), 0.f, 1.f);
		speedRatio = bIsAccelerating ? 1.f - speedRatio : speedRatio;
		Settings.Tilt.RuntimeTilt.Speed *= speedRatio;

		/* Correct tilting max angle by considering speed difference. */
		const float selectedAccelerationValue = bIsAccelerating ? accelerationAbsolute : brakingAbsolute;
		Settings.Tilt.RuntimeTilt.MaxAngle *= FMath::Clamp(selectedAccelerationValue / Settings.Tilt.RuntimeTilt.TiltAccelerationTarget, 0.f, 1.f);
		Settings.Tilt.RuntimeTilt.MaxAngle *= (bIsAccelerating ? movementDirectionMultiplier : -movementDirectionMultiplier);
	}

	/* Bump up tilt timer. */
	Settings.Tilt.Timer += DeltaTime;

	/* Calculate final tilt with oscillation formula. */
	Settings.Tilt.CurrentTilt = Settings.Tilt.RuntimeTilt.MaxAngle * FMath::Exp(-1.f * Settings.Tilt.RuntimeTilt.Damping * Settings.Tilt.Timer) * FMath::Sin(Settings.Tilt.RuntimeTilt.Speed * Settings.Tilt.Timer);
}

void UArcadeVehicleAnimationInstance::CalculateRoll(float DeltaTime)
{
	/* Cache vehicle vehicleSettings. */
	const FVehicleSettings& vehicleSettings = m_pVehicleMovementComponent->GetVehicleSettings();

	/* Calculate wheel slip by calculating ratio of the side linear velocity axis and value provided. */
	float wheelSlip = FMath::Clamp((m_pVehicleMovementComponent->GetLocalLinearVelocity().Y * KMH_MULTIPLIER) / Settings.Roll.RollDampingSpeed, -1.f, 1.f);
	wheelSlip *= Settings.Roll.MaxAngle;

	/* Calculate final roll value. */
	Settings.Roll.CurrentRoll = FMath::Clamp(wheelSlip, -Settings.Roll.MaxAngle, Settings.Roll.MaxAngle) * Settings.Roll.Strength;
}

void UArcadeVehicleAnimationInstance::AllocateWheels(const FVehicleSuspensionSettings& Suspension)
{
	if (Settings.Wheels.Registry.Num() == Suspension.Springs.Num())
	{
		return;
	}

	/* Empty array just in case. */
	Settings.Wheels.Registry.Empty();

	/* Fill array with default info. */
	FVehicleWheelAnimationInfo defaultInfo;
	Settings.Wheels.Registry.Init(defaultInfo, Suspension.Springs.Num());
}
