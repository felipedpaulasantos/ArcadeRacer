/** Created and owned by Furious Production LTD @ 2023. **/

#pragma once
#include "ArcadeVehicleAnimationSettings.generated.h"

/**
	Enumerator that allows to specify which side of the vehicle
	the wheel belongs to.
*/
UENUM(BlueprintType)
enum class VehicleWheelSide : uint8
{
	None,
	Left,
	Right
};

/**
	Additional structure to gather vehicle wheel data for the animation specifically.
*/
USTRUCT(BlueprintType)
struct ARCADEVEHICLESYSTEM_API FVehicleWheelAnimationInfo
{
	GENERATED_BODY()

	FVehicleWheelAnimationInfo();

	/** Stores latest up-to-date wheel rotation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Wheels)
	float Rotation;

	/** Stores latest up-to-date wheel offset in Z axis. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Wheels)
	float Offset;

	/** Stores latest up-to-date wheel swing amopunt. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Wheels)
	float Swing;

	/** Allows to specify which side of the vehicle this wheel lays on. It is not used currently, but can be. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Wheels)
	VehicleWheelSide Side;
	
	/** Static mesh component associated with this wheel. It can perfectly be valid to be null if not used. */
	UPROPERTY(BlueprintReadOnly, Category = Wheels)
	TWeakObjectPtr<UStaticMeshComponent> WheelMesh;

	/** Initial relative transform of the wheel mesh upon initialization. */
	UPROPERTY(BlueprintReadOnly, Category = Wheels)
	FTransform InitialWheelMeshTransform;

	/** Value grabbed when the wheels are allocated and taken from the vehicle settings. */
	UPROPERTY(BlueprintReadOnly, Category = Wheels)
	bool bIsSteeringWheel;
};

/**
	Helper structure that gathers all wheels animation-related
	settings of the vehicle animation instance.
*/
USTRUCT(BlueprintType)
struct ARCADEVEHICLESYSTEM_API FVehicleAnimationWheelsSettings
{
	GENERATED_BODY()

	FVehicleAnimationWheelsSettings();

	/** The most known up-to-date wheels rotation. If the wheels info is not filled out in the panel, system will make sure they match what is in the vehicle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Wheels)
	TArray<FVehicleWheelAnimationInfo> Registry;

	/** Runtime value that defines begin and end rotation of the wheel direction for smooth blending. */
	UPROPERTY()
	FVector2D DirectionBeginEnd;

	/** The most known up-to-date wheels direction. It is only exposed here to allow users to preview it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Wheels)
	float CurrentDirection;

	/** Defines max angle of the wheels turn. Angle is defined in degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Wheels)
	float MaxDirection;

	/** Defines wheels turn transition time, so how much time it takes for the wheels to perform full direction change. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Wheels)
	float TransitionTime;

	/** Runtime timer for the wheels direction blending. */
	UPROPERTY()
	float Timer;

	/** Runtime value of the time required for the blending. It is calculated on change based on how many degrees left there are. */
	UPROPERTY()
	float CurrentDuration;

	/** An optional flag. When it's true, the wheels will immediately stop rotating when braking. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Wheels)
	bool bStopRotationOnBraking;
};

/**
	Helper structure to gather all info about single
	vehicle body tilt setting for the total tilt settings.
*/
USTRUCT(BlueprintType)
struct ARCADEVEHICLESYSTEM_API FVehicleBodyTiltData
{
	GENERATED_BODY()

	FVehicleBodyTiltData();

	/** Defines maximum angle that the vehicle body can reach when using this tilt data. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tilt)
	float MaxAngle;

	/** Defines how fast tilting animation is performed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tilt)
	float Speed;

	/** Defines how tilt animation is damped. If zero, then it will just keep going. Higher values will stop sooner. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tilt)
	float Damping;

	/** 
		Defines acceleration value that needs to be reached in order to achieve full tilt angle.
		The closer vehicle acceleration is to this value, the more it multiplies tilt.
		It allows to have smaller tilts when for example using controller, where the input
		might not be fully 1 or 0.
		This takes acceleration value. Check acceleration curve in your vehicle to understand
		what kind of acceleration values you want here. Typically about 10 is good.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tilt)
	float TiltAccelerationTarget;

	/**
		This setting defines how current vehicle speed affects tilting animation.
		So for example, if the vehicle runs very fast, close to this value,
		the tilting speed will be very small. This allows to add more stability
		when running faster, than when at low speeds. So the way it works is,
		that the vehicle current speed will be divided by this value and clamped between 0 and 1.
		Then the speed of the oscillation of the tilt will be multiplied by this value.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tilt)
	float TiltDampingSpeed;
};

/**
	Helper structure that gathers all body tilt animation-related
	settings of the vehicle animation instance.
*/
USTRUCT(BlueprintType)
struct ARCADEVEHICLESYSTEM_API FVehicleAnimationTiltSettings
{
	GENERATED_BODY()

	FVehicleAnimationTiltSettings();

	/** 
		Tilt animation settings for the acceleration of the vehicle.
		Note that the acceleration can mean both forward or backward.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Acceleration)
	FVehicleBodyTiltData AccelerationTilt;

	/**
		Tilt animation settings for the braking of the vehicle.
		Note that braking in this case simply means releasing input, or accelerating
		in the opposite direction of the vehicle.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Braking)
	FVehicleBodyTiltData BrakingTilt;

	/** Runtime tilt data filled out at runtime from appropriate tilt type selected. */
	UPROPERTY()
	FVehicleBodyTiltData RuntimeTilt;

	/** The most up-to-date vehicle body tilt angle. It is only exposed here to allow users to preview it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tilt)
	float CurrentTilt;

	/** Runtime timer, that calculates oscillation time for sinus for the tilting animation. */
	UPROPERTY()
	float Timer;
};

/**
	Helper structure that gathers all body roll animation-related
	settings of the vehicle animation instance.
*/
USTRUCT(BlueprintType)
struct ARCADEVEHICLESYSTEM_API FVehicleAnimationRollSettings
{
	GENERATED_BODY()

	FVehicleAnimationRollSettings();

	/** The most up-to-date vehicle body roll angle. It is only exposed here to allow users to preview it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Roll)
	float CurrentRoll;

	/** Maximum roll angle that the vehicle can be in. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Roll)
	float MaxAngle;

	/** Strength of the roll. It is just simple multiplier of how much this effect takes place. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Roll)
	float Strength;

	/**
		This parameter defines angular speed of the vehicle required for achieving full roll angle.
		So the vehicle side-speed will be divided by this value, and when it reaches this value,
		full roll will be applied. Basically it just defines how much roll is given at certain speed.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Roll)
	float RollDampingSpeed;
};

/**
	Structure for grouping all of the vehicle animation settings together,
	and to make them easier to copy between blueprints.
*/
USTRUCT(BlueprintType)
struct ARCADEVEHICLESYSTEM_API FVehicleAnimationSettings
{
	GENERATED_BODY()

	/** All wheels settings for this vehicle animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Wheels)
	FVehicleAnimationWheelsSettings Wheels;

	/** All tilt settings for this vehicle animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tilt)
	FVehicleAnimationTiltSettings Tilt;

	/** All roll settings for this vehicle animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Roll)
	FVehicleAnimationRollSettings Roll;
};