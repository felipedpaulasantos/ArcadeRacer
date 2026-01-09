/** Created and owned by Furious Production LTD @ 2023. **/

#pragma once

#include "CoreMinimal.h"
#include "ArcadeVehicleAnimationSettings.h"
#include "Components/ActorComponent.h"
#include "Settings/ArcadeVehicleSettings.h"
#include "StaticArcadeVehicleAnimator.generated.h"

class UStaticArcadeVehicleMovementComponent;

/**
 * Component that is used to animate arcade vehicles, that use static mesh instead of skeletal.
 * It has exactly the same logic as skeletal version, but because static meshes can't have animations
 * this component handles the transform logic just like anim instance.
 */
UCLASS(BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class ARCADEVEHICLESYSTEM_API UStaticArcadeVehicleAnimator : public UActorComponent
{
	GENERATED_BODY()

public:
	UStaticArcadeVehicleAnimator();

	/** UActorComponent interface. */
	void BeginPlay() override;
	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	/** ~UActorComponent interface. */

	/**
	  * Returns vehicle movement component associated with this animator.
	  * It is not Pure node, because what it basically does is, it checks if we have valid instance,
	  * and if not, it will try to return one from the owner actor.
	  * It will throw error if there was not available vehicle movement.
	*/
	UFUNCTION(BlueprintCallable, Category = Movement)
	UStaticArcadeVehicleMovementComponent* GetVehicleMovementComponent();

	/** This actually applies animation values to the vehicle meshes. Implement logic here if needed. */
	UFUNCTION(BlueprintNativeEvent, Category = Animation)
	void ApplyAnimation();
	void ApplyAnimation_Implementation();
	
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

	/* Allocates wheel info array if empty. Returns true if allocated successfully. */
	bool AllocateWheels(const FVehicleSuspensionSettings& Suspension);
	
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
	UStaticArcadeVehicleMovementComponent* m_pVehicleMovementComponent;
};
