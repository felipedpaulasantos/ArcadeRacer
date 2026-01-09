/** Created and owned by Furious Production LTD @ 2023. **/

#pragma once
#include "ArcadeVehicleAnimationInstance.h"
#include "TankVehicleAnimationInstance.generated.h"

/**
	
*/

/**
	Base for the animation instances that are driven by the arcade vehicle of this system.
	Obviously, feel free to implement your own animation instance, but as a good base on how to use it,
	this one is pretty good introduction and in most cases, there is all you need.
	This implementation handles tank-like animations of the wheels.
*/
UCLASS(BlueprintType, Blueprintable)
class ARCADEVEHICLESYSTEM_API UTankVehicleAnimationInstance : public UArcadeVehicleAnimationInstance
{
	GENERATED_BODY()

public:
	UTankVehicleAnimationInstance();

protected:
	/** Calculates wheels rotation. */
	virtual void CalculateWheelsRotation(float DeltaTime);

protected:
	/** Multiplier for the wheel speed when in-place. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TankAnimation)
	float InPlaceWheelRotationMultiplier;

	/** Multiplier for the wheels rotation slowing when turning the tank in movement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TankAnimation)
	float WheelSideRotationMultiplier;

	/** Speed of the vehicle required for the wheels opposite direction rotation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TankAnimation, meta=(UIMin="0.0", ClampMin="0.0"))
	float VehicleSpeedForInverseWheelRotation;
};