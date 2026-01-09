/** Created and owned by Furious Production LTD @ 2023. **/

#include "Animations/StaticArcadeVehicleAnimator.h"
#include "Movement/StaticArcadeVehicleMovementComponent.h"
#include "Components/StaticMeshComponent.h"

UStaticArcadeVehicleAnimator::UStaticArcadeVehicleAnimator()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	
	m_bWasAccelerating = false;
	m_bWasBraking = false;
}

void UStaticArcadeVehicleAnimator::BeginPlay()
{
	Super::BeginPlay();

	/* Disallow ticking. */
	SetComponentTickEnabled(false);
	
	/* Make sure we have vehicle movement component. */
	if(!IsValid(GetVehicleMovementComponent()))
	{
		UE_LOG(LogArcadeVehicleMovement, Error, TEXT("Vehicle does not have movement component!"));
		return;
	}

	/* Make sure vehicle movement was initialized. */
	if(!IsValid(GetVehicleMovementComponent()->GetVisualsMesh()) || !IsValid(GetVehicleMovementComponent()->GetVehicleMesh()))
	{
		UE_LOG(LogArcadeVehicleMovement, Error, TEXT("Vehicle does not have movement initialized!"));
		return;
	}

	/* Cache vehicle vehicleSettings. */
	const FVehicleSettings& vehicleSettings = GetVehicleMovementComponent()->GetVehicleSettings();
	
	/* Allocate wheel rotations if they are not present. */
	if(AllocateWheels(vehicleSettings.Suspension))
	{
		/* Enable ticking as we have successfully allocated wheels. */
		SetComponentTickEnabled(true);
	}
}

void UStaticArcadeVehicleAnimator::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	/* Must have valid vehicle. */
	if (!IsValid(GetVehicleMovementComponent()))
	{
		return;
	}

	/* Perform wheels-related calculations. */
	CalculateWheelsDirection(DeltaTime);
	CalculateWheelsRotation(DeltaTime);
	CalculateWheelsOffsets(DeltaTime);

	/* Perform body-related calculations. */
	CalculateTilt(DeltaTime);
	CalculateRoll(DeltaTime);

	/* Apply animation in the end. */
	ApplyAnimation();
}

UStaticArcadeVehicleMovementComponent* UStaticArcadeVehicleAnimator::GetVehicleMovementComponent()
{
	if (!IsValid(m_pVehicleMovementComponent))
	{
		m_pVehicleMovementComponent = GetOwner()->FindComponentByClass<UStaticArcadeVehicleMovementComponent>();
	}

	return m_pVehicleMovementComponent;
}

void UStaticArcadeVehicleAnimator::CalculateWheelsDirection(float DeltaTime)
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

void UStaticArcadeVehicleAnimator::CalculateWheelsRotation(float DeltaTime)
{
	/* Rotation multiplier based on the vehicle direction. */
	const float rotationDirection = m_pVehicleMovementComponent->IsMovingBackward() ? 1.f : -1.f;

	/* Cache vehicle vehicleSettings. */
	const FVehicleSettings& vehicleSettings = m_pVehicleMovementComponent->GetVehicleSettings();

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

void UStaticArcadeVehicleAnimator::CalculateWheelsOffsets(float DeltaTime)
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

void UStaticArcadeVehicleAnimator::CalculateTilt(float DeltaTime)
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

void UStaticArcadeVehicleAnimator::CalculateRoll(float DeltaTime)
{
	/* Cache vehicle vehicleSettings. */
	const FVehicleSettings& vehicleSettings = m_pVehicleMovementComponent->GetVehicleSettings();

	/* Calculate wheel slip by calculating ratio of the side linear velocity axis and value provided. */
	float wheelSlip = FMath::Clamp((m_pVehicleMovementComponent->GetLocalLinearVelocity().Y * KMH_MULTIPLIER) / Settings.Roll.RollDampingSpeed, -1.f, 1.f);
	wheelSlip *= Settings.Roll.MaxAngle;

	/* Calculate final roll value. */
	Settings.Roll.CurrentRoll = FMath::Clamp(wheelSlip, -Settings.Roll.MaxAngle, Settings.Roll.MaxAngle) * Settings.Roll.Strength;
}

bool UStaticArcadeVehicleAnimator::AllocateWheels(const FVehicleSuspensionSettings& Suspension)
{
	if(Settings.Wheels.Registry.Num() != Suspension.Springs.Num())
	{
		UE_LOG(LogArcadeVehicleMovement, Error, TEXT("Vehicle animator does not have allocated wheels for animation!"));
		return false;
	}

	/* Grab visuals mesh. */
	UStaticMeshComponent* pVehicleMesh = GetVehicleMovementComponent()->GetVisualsMesh();

	/* Grab all meshes attached to the vehicle mesh, those are potential wheels. */
	TArray<USceneComponent*> potentialWheels = pVehicleMesh->GetAttachChildren();

	/* Iterate over springs. */
	for(int32 springIndex = 0; springIndex < Suspension.Springs.Num(); ++springIndex)
	{
		/* Cache iterated spring. */
		const FVehicleSuspensionSpring& spring = Suspension.Springs[springIndex];
		FVehicleWheelAnimationInfo& wheel = Settings.Wheels.Registry[springIndex];
		
		/* Iterate over potential wheels to find one, that matches this spring. */
		USceneComponent** pPotentialWheel = potentialWheels.FindByPredicate([spring](USceneComponent* element)
		{
			return element->GetAttachSocketName().IsEqual(spring.BoneName);
		});

		/* Bail if invalid. */
		if(pPotentialWheel == nullptr)
		{
			UE_LOG(LogArcadeVehicleMovement, Error, TEXT("Vehicle animator couldn't find wheel meshes!"));
			return false;
		}
		
		/* If found, set it to the wheel registry. */
		wheel.WheelMesh = Cast<UStaticMeshComponent>(*pPotentialWheel);
		wheel.InitialWheelMeshTransform = Settings.Wheels.Registry[springIndex].WheelMesh->GetRelativeTransform();
		wheel.bIsSteeringWheel = spring.bIsSteeringWheel;
	}

	/* We have success at this point. */
	return true;
}

void UStaticArcadeVehicleAnimator::ApplyAnimation_Implementation()
{
	/* Grab visuals mesh. */
	UStaticMeshComponent* pVehicleMesh = GetVehicleMovementComponent()->GetVisualsMesh();

	/* Apply suspension rotation for the tilt and roll. */
	FRotator suspensionRotation = FRotator::ZeroRotator;
	suspensionRotation.Roll = Settings.Roll.CurrentRoll;
	suspensionRotation.Pitch = Settings.Tilt.CurrentTilt;
	pVehicleMesh->SetRelativeRotation(suspensionRotation);

	/* Rotate wheels. */
	for(FVehicleWheelAnimationInfo& wheel : Settings.Wheels.Registry)
	{
		/* Prepare rotator. Use wheel direction if the wheel is steering wheel. */
		FRotator wheelRotation = wheel.InitialWheelMeshTransform.GetRotation().Rotator();

		/* Invert rotation if this is the right side of the vehicle. */
		wheelRotation.Roll = 0.f;
		wheelRotation.Pitch = wheel.Rotation * (wheel.Side == VehicleWheelSide::Right ? -1.f : 1.f);

		/* If this is steering wheel, perform steering. */
		if(wheel.bIsSteeringWheel)
		{
			wheelRotation.Yaw += Settings.Wheels.CurrentDirection;
		}

		/* Apply wheel swing rotation. */
		FRotator rollRotator(0.f, 0.f, wheel.Swing);
		wheelRotation = (rollRotator.Quaternion() * wheelRotation.Quaternion()).Rotator();

		/* Apply wheel rotation. */
		wheel.WheelMesh->SetRelativeRotation(wheelRotation);

		/* Prepare wheel offset. */
		FVector wheelOffset = wheel.InitialWheelMeshTransform.GetLocation();
		wheelOffset.Z += wheel.Offset;

		/* Apply wheel offset. */
		wheel.WheelMesh->SetRelativeLocation(wheelOffset);

		
	}
}
