/** Created and owned by Furious Production LTD @ 2023. **/

#include "Camera/ArcadeVehicleCameraController.h"
#include "Movement/ArcadeVehicleMovementComponentBase.h"
#include "CineCameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

FCameraSnappingData::FCameraSnappingData()
{
	SnapRotation = FRotator::ZeroRotator;
	State = CameraSnappingSequenceState::None;	
	BlockTimer = 0.f;
}

UArcadeVehicleCameraController::UArcadeVehicleCameraController()
{
	/* Camera controller should always use absolute rotation to avoid up-side-down camera when entering the vehicle. */
	SetUsingAbsoluteRotation(true);

	/* Mostly constant values. */
	CameraPossessingBlendTime = 1.15f;
	bUseRotationSnapping = true;
	bUsePitchHelper = true;
	bUsePawnControlRotation = false;
	RotationSnappingDelay = 1.f;
	YawSnappingSpeed = 0.1f;
	PitchSnappingSpeed = 0.1f;
	RotationSnappingTolerance = 0.5f;
	PitchThreshold = 10.f;
	PitchUpwardAngularOffset = 10.f;
	PitchDownwardAngularOffset = 10.f;

	/* Runtime values. */
	m_ownerPitchState = CameraOwnerPitchState::Flat;
	m_ownerSpeed = 0.f;
	m_bIsOwnerSpeedDirty = false;

	/* Inherit only yaw. */
	bInheritPitch = false;
	bInheritRoll = false;
	bInheritYaw = true;
}

void UArcadeVehicleCameraController::BeginPlay()
{
	Super::BeginPlay();
	
	/* Mark fov effects dirty at start to put the initial values in. */
	m_bIsOwnerSpeedDirty = true;

	/* Grab initial snap rotation for the yaw and pitch snapping. They are initially the same. */
	m_yawSnappingData.SnapRotation = GetRelativeRotation();
	m_pitchSnappingData.SnapRotation = GetRelativeRotation();

	/* Turn pitch into 180. */
	RotatorPitchTo180(m_yawSnappingData.SnapRotation);
	RotatorPitchTo180(m_pitchSnappingData.SnapRotation);

	/* Find target local player controller. */
	m_pLocalPlayerController = GetWorld()->GetFirstPlayerController();
	EvaluateInitializationFlag();
}

void UArcadeVehicleCameraController::Activate(bool bReset /*= false*/)
{
	/* Reset snapping values upon activation. */
	m_yawSnappingData.State = CameraSnappingSequenceState::None;
	m_yawSnappingData.BlockTimer = 0.f;
	//
	m_pitchSnappingData.State = CameraSnappingSequenceState::None;
	m_pitchSnappingData.BlockTimer = 0.f;
	//
	m_ownerPitchState = CameraOwnerPitchState::Flat;

	Super::Activate();
}

void UArcadeVehicleCameraController::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	/* Must be initialized and owned locally. */
	if(!m_bIsControllerInitialized || !IsLocallyControlled())
	{
		return;
	}

	/* Grab acceleration input from the movement component. */
	const float accelerationInput = FMath::Abs(m_pMovementComponent->GetLastAppliedAcceleration());

	/* Begin or stop snapping sequence. */
	if(accelerationInput > 0.f)
	{
		BeginSnappingSequence();
	}
	else
	{
		StopSnappingSequence();
	}
	
	/* Calculate snapping if enabled. */
	if(bUseRotationSnapping)
	{
		/* Contribute yaw snapping. */
		CalculateYawSnapping(DeltaTime);

		/* Contribute pitch snapping. */
		CalculatePitchSnapping(DeltaTime);
	}

	/* Calculate various camera effects in the end of the frame. */
	CalculateCameraSpeedEffects(DeltaTime);
}

void UArcadeVehicleCameraController::InitializeCameras(UArcadeVehicleMovementComponentBase* MovementComponent, UCineCameraComponent* FrontCamera, UCineCameraComponent* RearCamera, USpringArmComponent* RearCameraArm)
{
	m_pMovementComponent = MovementComponent;
	m_pFrontCamera = FrontCamera;
	m_pRearCamera = RearCamera;
	m_pRearCameraArm = RearCameraArm;
	m_pLocalPlayerController = GetWorld()->GetFirstPlayerController();
	EvaluateInitializationFlag();
}

void UArcadeVehicleCameraController::ActivateFrontCamera()
{
	/* Must be initialized. */
	if(!m_bIsControllerInitialized)
	{
		return;
	}
	
	m_pRearCamera->Deactivate();
	m_pRearCameraArm->Deactivate();
	//
	m_pFrontCamera->Activate(false);
	Activate(false);
}

void UArcadeVehicleCameraController::ActivateRearCamera()
{
	/* Must be initialized. */
	if(!m_bIsControllerInitialized)
	{
		return;
	}
	
	m_pFrontCamera->Deactivate();
	Deactivate();
	//
	m_pRearCamera->Activate(false);
	m_pRearCameraArm->Activate(false);
}

void UArcadeVehicleCameraController::DeactivateAllCameras()
{
	/* Must be initialized. */
	if(!m_bIsControllerInitialized)
	{
		return;
	}
	
	m_pRearCamera->Deactivate();
	m_pRearCameraArm->Deactivate();
	m_pFrontCamera->Deactivate();
	Deactivate();
}

bool UArcadeVehicleCameraController::IsFrontCameraActive()
{
	return m_pFrontCamera->IsActive();
}

void UArcadeVehicleCameraController::AddYawInput(float Value)
{
	/* Must be initialized. */
	if(!m_bIsControllerInitialized)
	{
		return;
	}
	
	/* Only if frontal camera is active. */
	if(!IsFrontCameraActive())
	{
		return;
	}

	/* Apply underlying controller input. */
	m_pLocalPlayerController->AddYawInput(Value);

	/* If not zeroed, block snapping sequence. */
	if(Value != 0.f)
	{
		BlockSnappingSequence();
	}
}

void UArcadeVehicleCameraController::AddPitchInput(float Value)
{
	/* Must be initialized. */
	if(!m_bIsControllerInitialized)
	{
		return;
	}
	
	/* Only if frontal camera is active. */
	if(!IsFrontCameraActive())
	{
		return;
	}

	/* Apply underlying controller input. */
	m_pLocalPlayerController->AddPitchInput(Value);

	/* If not zeroed, block snapping sequence. */
	if(Value != 0.f)
	{
		BlockSnappingSequence();
	}
}

void UArcadeVehicleCameraController::BeginSnappingSequence()
{
	/* Check if blocked. */
	if (m_yawSnappingData.State == CameraSnappingSequenceState::Blocked)
	{
		return;
	}

	/* If not blocked, we also want to bail out if state is not none, because we do not want to reset interpolation or snapping. */
	if (m_yawSnappingData.State != CameraSnappingSequenceState::None)
	{
		return;
	}

	/* Mark as interpolating. */
	m_yawSnappingData.State = CameraSnappingSequenceState::Interpolating;
}

void UArcadeVehicleCameraController::StopSnappingSequence()
{
	/* Mark state as none if it was interpolating. */
	if (m_yawSnappingData.State == CameraSnappingSequenceState::Interpolating)
	{
		m_yawSnappingData.State = CameraSnappingSequenceState::None;
	}
}

void UArcadeVehicleCameraController::BlockSnappingSequence()
{
	/* Enable camera lag back. */
	bEnableCameraRotationLag = true;
	
	/* Put yaw and pitch state to blocked. */
	m_yawSnappingData.State = CameraSnappingSequenceState::Blocked;
	m_pitchSnappingData.State = CameraSnappingSequenceState::Blocked;

	/* Set new block time. */
	m_yawSnappingData.BlockTimer = RotationSnappingDelay;
	m_pitchSnappingData.BlockTimer = RotationSnappingDelay;
}

void UArcadeVehicleCameraController::ClearSnappedState()
{
	if (m_yawSnappingData.State == CameraSnappingSequenceState::Snapped)
	{
		m_yawSnappingData.State = CameraSnappingSequenceState::None;
	}

	if (m_pitchSnappingData.State == CameraSnappingSequenceState::Snapped)
	{
		m_pitchSnappingData.State = CameraSnappingSequenceState::None;
	}
}

void UArcadeVehicleCameraController::UpdateOwnerSpeed(float Speed)
{
	/* Check if speed has changed. */
	if (m_ownerSpeed == Speed)
	{
		return;
	}

	/* Set new speed value. */
	m_ownerSpeed = Speed;

	/* Mark dirty. */
	m_bIsOwnerSpeedDirty = true;
}

void UArcadeVehicleCameraController::UpdateTurnAngle(float TurnAngle)
{
	m_ownerTurnAngle = TurnAngle;
}

FRotator UArcadeVehicleCameraController::GetDesiredRotation() const
{
	/* If using provided controller and it is valid. */
	if (IsValid(m_pLocalPlayerController))
	{
		/* Sample rotation form the controller. */
		return m_pLocalPlayerController->GetControlRotation();
	}
	
	/* Proceed the regular way. */
	return GetComponentRotation();
}

void UArcadeVehicleCameraController::RotatorPitchTo180(FRotator& OutRotator)
{
	if (OutRotator.Pitch > 180.f)
	{
		float newValue = OutRotator.Pitch - 180.f;
		OutRotator.Pitch = (-180.f) + newValue;
	}
	else if (OutRotator.Pitch < -180.f)
	{
		float newValue = (-180.f) - OutRotator.Pitch;
		OutRotator.Pitch = 180.f - newValue;
	}
}

void UArcadeVehicleCameraController::EvaluateInitializationFlag()
{
	/* Make sure we set the controller valid. */
	m_bIsControllerInitialized =
		IsValid(m_pMovementComponent) &&
			IsValid(m_pFrontCamera) &&
				IsValid(m_pRearCamera) &&
					IsValid(m_pRearCameraArm) &&
						IsValid(m_pLocalPlayerController);
}

void UArcadeVehicleCameraController::CalculateYawSnapping(float DeltaTime)
{
	/* If snapping state is none, do nothing here. */
	if (m_yawSnappingData.State == CameraSnappingSequenceState::None)
	{
		return;
	}

	/* If currently blocked. */
	if (m_yawSnappingData.State == CameraSnappingSequenceState::Blocked)
	{
		/* Bring down the block timer. */
		m_yawSnappingData.BlockTimer -= DeltaTime;

		/* When the timer reaches 0. */
		if (m_yawSnappingData.BlockTimer <= 0.f)
		{
			/* Re-allow any new snap sequences. */
			m_yawSnappingData.State = CameraSnappingSequenceState::None;
		}

		/* Bail out. */
		return;
	}

	/* If currently interpolating, those calculations are done here. */
	if (m_yawSnappingData.State == CameraSnappingSequenceState::Interpolating)
	{
		/* Grab local rotation of the control. */
		const FRotator currentRotation = GetOwner()->GetActorTransform().InverseTransformRotation(m_pLocalPlayerController->GetControlRotation().Quaternion()).Rotator();
		const FRotator targetRotation = m_yawSnappingData.SnapRotation;

		/* Calculate yaw distance to finalize. */
		const float angularDistance = FMath::Abs(currentRotation.Yaw - targetRotation.Yaw);

		/* Check if yaw needs to contribute. */
		const bool bContributeYaw = !FMath::IsNearlyZero(angularDistance, RotationSnappingTolerance);

		/* Contribute yaw if needed. */
		if (bContributeYaw)
		{
			/* Calculate temporary snapping speed, so we have nice looking snapping even if rotation is way off, and so we don't wait forever. */
			float frameYawSpeed = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 180.f), FVector2D(YawSnappingSpeed, 1.f), angularDistance);

			if (currentRotation.Yaw < targetRotation.Yaw)
			{
				/* Add control yaw. */
				m_pLocalPlayerController->AddYawInput(frameYawSpeed);
			}
			else
			{
				/* Remove control yaw. */
				m_pLocalPlayerController->AddYawInput(-frameYawSpeed);
			}
		}
		/* If yaw does not contribute, conclude interpolation. */
		else
		{
			m_yawSnappingData.State = CameraSnappingSequenceState::Snapped;
		}
	}
	/* If currently snapped, simply copy owner rotation to the controller. */
	else if (m_yawSnappingData.State == CameraSnappingSequenceState::Snapped)
	{
		/* Derive yaw rotation from the owner actor. */
		FRotator snapRotation = m_pLocalPlayerController->GetControlRotation();
		snapRotation.Yaw = GetOwner()->GetActorRotation().Yaw;
		snapRotation.Roll = 0.f;
		m_pLocalPlayerController->SetControlRotation(snapRotation);
	}
}

FRotator UArcadeVehicleCameraController::CalculateTargetPitchRotation()
{
	FRotator targetRotation = m_pitchSnappingData.SnapRotation;
	if (m_ownerPitchState == CameraOwnerPitchState::Down)
	{
		targetRotation.Pitch -= PitchDownwardAngularOffset;
	}
	else if (m_ownerPitchState == CameraOwnerPitchState::Up)
	{
		targetRotation.Pitch += PitchUpwardAngularOffset;
	}
	return targetRotation;
}

void UArcadeVehicleCameraController::CalculatePitchSnapping(float DeltaTime)
{
	/* Only if pitch helper is used. */
	if (!bUsePitchHelper)
	{
		return;
	}

	/* Check this frame owner pitch state. */
	const float ownerPitch = GetOwner()->GetActorRotation().Pitch;
	CameraOwnerPitchState newPitchState = m_ownerPitchState;
	if (ownerPitch > PitchThreshold)
	{
		newPitchState = CameraOwnerPitchState::Up;
	}
	else if (ownerPitch < -PitchThreshold)
	{
		newPitchState = CameraOwnerPitchState::Down;
	}
	else
	{
		newPitchState = CameraOwnerPitchState::Flat;
	}

	/* If pitch state has changed, assign new pitch. */
	if (m_ownerPitchState != newPitchState)
	{
		/* Assign new state. */
		m_ownerPitchState = newPitchState;

		/* If YAW is snapped or is during interpolation, we will only contribute then. */
		if (m_yawSnappingData.State == CameraSnappingSequenceState::Snapped || m_yawSnappingData.State == CameraSnappingSequenceState::Interpolating)
		{
			/* Mark as interpolating. */
			m_pitchSnappingData.State = CameraSnappingSequenceState::Interpolating;
		}
	}

	/* Calculate blocked state. */
	if (m_pitchSnappingData.State == CameraSnappingSequenceState::Blocked)
	{
		/* Bring down the block timer. */
		m_pitchSnappingData.BlockTimer -= DeltaTime;

		/* When the timer reaches 0. */
		if (m_pitchSnappingData.BlockTimer <= 0.f)
		{
			/* Re-allow any new snap sequences. */
			m_pitchSnappingData.State = CameraSnappingSequenceState::None;
		}

		/* Bail out. */
		return;
	}

	/* Calculate interpolating state. */
	if (m_pitchSnappingData.State == CameraSnappingSequenceState::Interpolating)
	{
		/* Grab local pitch rotation to target and the control rotation. */
		const FRotator currentRotation = GetOwner()->GetActorTransform().InverseTransformRotation(m_pLocalPlayerController->GetControlRotation().Quaternion()).Rotator();
		const FRotator targetRotation = CalculateTargetPitchRotation();

		/* Check if pitch needs to contribute. */
		const bool bContributePitch = !FMath::IsNearlyEqual(FMath::Abs(currentRotation.Pitch - targetRotation.Pitch), 0.f, RotationSnappingTolerance);

		/* Contribute pitch if needed. */
		if (bContributePitch)
		{
			if (currentRotation.Pitch < targetRotation.Pitch)
			{
				/* Add control pitch. */
				m_pLocalPlayerController->AddPitchInput(-PitchSnappingSpeed);
			}
			else
			{
				/* Remove control pitch. */
				m_pLocalPlayerController->AddPitchInput(PitchSnappingSpeed);
			}
		}
		/* If pitch does not contribute, conclude interpolation. */
		else
		{
			/* Grab current controller rotation in local space. */
			FRotator snapRotation = GetOwner()->GetActorTransform().InverseTransformRotation(m_pLocalPlayerController->GetControlRotation().Quaternion()).Rotator();

			/* Contribute pitch. */
			snapRotation.Pitch = targetRotation.Pitch;

			/* Transform back to world. */
			snapRotation = GetOwner()->GetActorTransform().TransformRotation(snapRotation.Quaternion()).Rotator();
			snapRotation.Roll = 0.f;

			/* Apply final rotation. */
			m_pLocalPlayerController->SetControlRotation(snapRotation);

			/* Mark as snapped. */
			m_pitchSnappingData.State = CameraSnappingSequenceState::Snapped;
		}
	}
}

void UArcadeVehicleCameraController::CalculateCameraSpeedEffects(float DeltaTime)
{
	/* If not dirty, do not calculate. */
	if (!m_bIsOwnerSpeedDirty)
	{
		return;
	}

	/* Un-mark dirty. */
	m_bIsOwnerSpeedDirty = false;
	
	/* Process focal length. */
	const float focalLengthScale = FMath::GetMappedRangeValueClamped(FocalLengthSpeedRange, FocalLengthValuesRange, m_ownerSpeed);

	/* Apply focal length. */
	m_pFrontCamera->CurrentFocalLength = focalLengthScale;
	m_pRearCamera->CurrentFocalLength = focalLengthScale;

	/* Apply camera to vehicle distance shortening along with focal length. */
	TargetArmLength = FMath::GetMappedRangeValueClamped(CameraApproachSpeedRange, CameraApproachValuesRange, m_ownerSpeed);
}

bool UArcadeVehicleCameraController::IsLocallyControlled() const
{
	return IsValid(m_pLocalPlayerController) && m_pLocalPlayerController->GetPawn() == GetOwner();
}