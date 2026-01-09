/** Created and owned by Furious Production LTD @ 2023. **/

#include "Settings/ArcadeVehicleSettings.h"
#include "Curves/CurveFloat.h"

FVehiclePhysicsSettings::FVehiclePhysicsSettings()
{
	EnableCustomGravity = false;
	StabilizationForce = 30.f;
	FrictionForce = 0.7f;
	FrictionForceThreshold = 5.f;
	TotalFrictionSpeedThreshold = 5.f;
	MovementDirectionTolerance = 0.f;
	PhysicsCorrectionExponential = 0.9f;
	PhysicsLocationSnapDistance = 150.f;
	PhysicsRotationSnapDistance = 3.f;
	bEnhancePhysicsCorrection = false;
}

FVehicleEngineSettings::FVehicleEngineSettings()
{
	AccelerationCurve = nullptr;
	ReversingCurve = nullptr;
	EngineBrakingCurve = nullptr;
	BrakingCurve = nullptr;
	
	MaxSpeed = 180.f;
	MaxReverseSpeed = 50.f;
	bScaleAccelerationByDriveWheels = true;
}

FVehicleSteeringSettings::FVehicleSteeringSettings()
{
	SteeringCurve = nullptr;

	AllowSteeringWhileBraking = false;
	SteeringDamping = 0.5f;
	LinearDamping = 1.f;
	AngularDamping = 1.f;
	DriftAdherencePercentage = 0.1f;
	DriftRotationPercentage = 0.1f;
	DriftRecoverySpeed = 0.1f;
	DriftMinSpeed = 30.f;
}

FVehicleSuspensionSpringTrace::FVehicleSuspensionSpringTrace()
{
	BeginLocation = FVector::ZeroVector;
	EndLocation = FVector::ZeroVector;
	Normal = FVector::ZeroVector;
	Distance = 0.f;
	IsHitValid = false;
}

FVehicleSuspensionSpring::FVehicleSuspensionSpring()
{
	BoneName = NAME_None;
	SpringForce = 0.f;
	TargetHeight = 0.f;
	Damping = 0.f;
	bIsSteeringWheel = false;
	bIsDriveWheel = true;
	WheelRadius = 35.f;
	SwingPivot = 0.5f;
	SwingMinMax = FVector2D::ZeroVector;
	MinMaxOffsetZ = FVector2D::ZeroVector;
	Location = FVector::ZeroVector;
	WheelOffset = 0.f;
	CurrentSwing = 0.f;
	LatestSpringForce = FVector::ZeroVector;
}

FVehicleSuspensionSettings::FVehicleSuspensionSettings()
{
	CollisionChannels.Add(ECC_WorldStatic);
	TraceLength = 0.f;
	TraceUpOffset = 50.f;
	TraceThickness = 0.f;
	SuspensionParentBoneName  = FName(NAME_None);
	SuspensionParentBoneIndex = INDEX_NONE;
	EnableGroundSnapping = false;
	EnableSuspensionStabilization = false;
	SuspensionStabilizationMultiplier = 5.f;
}

FVehicleAdvancedSettings::FVehicleAdvancedSettings()
{
	bDisablePhysicsInSequencer = true;
	bEnableSuspension = true;
	bEnableAdherence = true;
	bEnableAcceleration = true;
	bEnableTurning = true;
	bEnableFriction = true;
}

FVehicleSettings::FVehicleSettings()
{
}

FVehicleWheelsRuntimeInfo::FVehicleWheelsRuntimeInfo()
{
	SteeringWheelsCount = 0;
	DriveWheelsCount = 0;
	SteeringWheelsOnGround = 0;
	DriveWheelsOnGround = 0;
}

float FVehicleWheelsRuntimeInfo::GetDriveWheelsMultiplier() const
{
	/* Avoid division by zero. */
	if(DriveWheelsCount < 1)
	{
		return 0.f;
	}

	const float fTotal = static_cast<float>(DriveWheelsCount);
	const float fOnGround = static_cast<float>(DriveWheelsOnGround);
	return fOnGround / fTotal;
}

float FVehicleWheelsRuntimeInfo::GetTotalWheelsMultiplier() const
{
	/* Establish wheel counts. */
	const int32 allWheels = DriveWheelsCount + SteeringWheelsCount;
	const int32 wheelsOnGround = DriveWheelsOnGround + SteeringWheelsOnGround;

	/* Skip if no wheels are on ground. */
	if(allWheels < 1)
	{
		return 0.f;
	}

	const float fTotal = static_cast<float>(allWheels);
	const float fOnGround = static_cast<float>(wheelsOnGround);
	return fOnGround / fTotal;
}
