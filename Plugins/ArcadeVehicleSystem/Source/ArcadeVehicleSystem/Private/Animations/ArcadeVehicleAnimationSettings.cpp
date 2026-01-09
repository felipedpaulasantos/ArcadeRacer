/** Created and owned by Furious Production LTD @ 2023. **/

#include "Animations/ArcadeVehicleAnimationSettings.h"

FVehicleWheelAnimationInfo::FVehicleWheelAnimationInfo()
{
	Rotation = 0.f;
	Offset = 0.f;
	Swing = 0.f;
	Side = VehicleWheelSide::None;
	bIsSteeringWheel = false;
}

FVehicleAnimationWheelsSettings::FVehicleAnimationWheelsSettings()
{
	/* Initialize exposed values. */
	CurrentDirection = 0.f;
	MaxDirection = 50.f;
	TransitionTime = 0.3f;
	bStopRotationOnBraking = true;

	/* Initialize runtime values. */
	DirectionBeginEnd = FVector2D::ZeroVector;
	Timer = 0.f;
	CurrentDuration = 0.f;
}

FVehicleBodyTiltData::FVehicleBodyTiltData()
{
	MaxAngle = 10.f;
	Speed = 10.f;
	Damping = 5.f;
	TiltAccelerationTarget = 10.f;
	TiltDampingSpeed = 150.f;
}

FVehicleAnimationTiltSettings::FVehicleAnimationTiltSettings()
{
	CurrentTilt = 0.f;
	Timer = 0.f;
}

FVehicleAnimationRollSettings::FVehicleAnimationRollSettings()
{
	CurrentRoll = 0.f;
	MaxAngle = 7.f;
	Strength = 5.f;
	RollDampingSpeed = 150.f;
}
