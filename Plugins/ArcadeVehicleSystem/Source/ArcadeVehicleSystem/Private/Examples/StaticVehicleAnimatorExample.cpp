// Fill out your copyright notice in the Description page of Project Settings.

#include "Examples/StaticVehicleAnimatorExample.h"

UStaticVehicleAnimatorExample::UStaticVehicleAnimatorExample()
{
	InitializeAnimatorSettings();
}

void UStaticVehicleAnimatorExample::InitializeAnimatorSettings()
{
	/* Setup wheels settings. */
	Settings.Wheels.Registry.AddDefaulted(4);
	Settings.Wheels.Registry[0].Side = VehicleWheelSide::Left;
	//
	Settings.Wheels.Registry[1].Side = VehicleWheelSide::Right;
	//
	Settings.Wheels.Registry[2].Side = VehicleWheelSide::Left;
	//
	Settings.Wheels.Registry[3].Side = VehicleWheelSide::Right;
	//
	Settings.Wheels.MaxDirection = 50.f;
	Settings.Wheels.TransitionTime = 0.3f;
	Settings.Wheels.bStopRotationOnBraking = true;

	/* Setup tilt settings. */
	Settings.Tilt.AccelerationTilt.MaxAngle = 30.f;
	Settings.Tilt.AccelerationTilt.Speed = 2.f;
	Settings.Tilt.AccelerationTilt.Damping = 4.f;
	Settings.Tilt.AccelerationTilt.TiltAccelerationTarget = 10.f;
	Settings.Tilt.AccelerationTilt.TiltDampingSpeed = 150.f;
	//
	Settings.Tilt.BrakingTilt.MaxAngle = 20.f;
	Settings.Tilt.BrakingTilt.Speed = 0.6f;
	Settings.Tilt.BrakingTilt.Damping = 4.f;
	Settings.Tilt.BrakingTilt.TiltAccelerationTarget = 30.f;
	Settings.Tilt.BrakingTilt.TiltDampingSpeed = 50.f;

	/* Setup roll settings. */
	Settings.Roll.MaxAngle = 3.5f;
	Settings.Roll.Strength = 5.f;
	Settings.Roll.RollDampingSpeed = 150.f;
}