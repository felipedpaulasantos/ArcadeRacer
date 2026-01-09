/** Created and owned by Furious Production LTD @ 2023. **/

#pragma once
#include "Animation/AnimInstance.h"
#include "ArcadeVehicleAnimationSettings.h"
#include "Settings/ArcadeVehicleSettings.h"
#include "ArcadeVehicleAnimationInstance.generated.h"

/**
	Base for the animation instances that are driven by the arcade vehicle of this system.
	Obviously, feel free to implement your own animation instance, but as a good base on how to use it,
	this one is pretty good introduction and in most cases, there is all you need.
*/
UCLASS(BlueprintType, Blueprintable)
class ARCADEVEHICLESYSTEM_API UArcadeVehicleAnimationInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UArcadeVehicleAnimationInstance();

	/** UAnimInstance interface. */
	void NativeUpdateAnimation(float DeltaSeconds) override;
	/** ~UAnimInstance interface. */

	/**
		Returns vehicle movement component associated with this animation instance.
		It is not Pure node, because what it basically does is, it checks if we have valid instance,
		and if not, it will try to return one from the owner actor.
		It will throw error if there was not available vehicle movement.
	*/
	UFUNCTION(BlueprintCallable, Category = Movement)
	UArcadeVehicleMovementComponent* GetVehicleMovementComponent();

protected:
	/** Calculates wheels direction. */
	virtual void CalculateWheelsDirection(float DeltaTime);

	/** Calculates wheels rotation. */
	virtual void CalculateWheelsRotation(float DeltaTime);

	/** Calculates all wheels offsets from the suspension data. */
	virtual void CalculateWheelsOffsets(float DeltaTime);

	/** Calculates tilt animation. */
	virtual void CalculateTilt(float DeltaTime);

	/** Calculates roll animation. */
	virtual void CalculateRoll(float DeltaTime);

	/* Allocates wheel info array if empty. */
	void AllocateWheels(const FVehicleSuspensionSettings& Suspension);

protected:
	/** Total settings for this animation instance of the vehicle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	FVehicleAnimationSettings Settings;
	
	/** Cached vehicle acceleration flag to see when it changes. */
	UPROPERTY()
	bool m_bWasAccelerating;
	
	/** Cached vehicle braking flag to see when it changes. */
	UPROPERTY()
	bool m_bWasBraking;

	/** Arcade vehicle movement component that calculates the movement that will then be animated via this anim instance. */
	UPROPERTY()
	UArcadeVehicleMovementComponent* m_pVehicleMovementComponent;
};