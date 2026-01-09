/** Created and owned by Furious Production LTD @ 2023. **/

#pragma once
#include "CoreMinimal.h"
#include "ArcadeVehicleMovementComponentBase.h"
#include "StaticArcadeVehicleMovementComponent.generated.h"

class UStaticArcadeVehicleAnimator;
class UStaticMeshComponent;

/**
* Implements arcade vehicle movement component logic for
* the vehicles that are created using Static Mesh Component.
* If your vehicles use Skeletal Mesh Component, please use different movement
* component for it.
* This component unlike its skeletal version, uses static mesh sockets
* to locate the wheel springs.
*/
UCLASS(BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class ARCADEVEHICLESYSTEM_API UStaticArcadeVehicleMovementComponent : public UArcadeVehicleMovementComponentBase
{
	GENERATED_BODY()

public:
	UStaticArcadeVehicleMovementComponent();
	
	/** UArcadeVehicleMovementComponentBase interface. */
	bool InitializeVehicleMovement() override;
	bool RegisterSuspensionSprings() override;
	void GetWheelsBaseTransform(FTransform& OutTransform) const override;
	/** ~UArcadeVehicleMovementComponentBase interface. */

	/**
	 * Returns static mesh used for visual aspects
	 * of this vehicle.
	 */
	UFUNCTION(BlueprintPure, Category = VehicleMesh)
	UStaticMeshComponent* GetVisualsMesh();

	/**
	 * Allows to set static animator class for this component.
	 * This class is used to animate vehicle suspension and wheels
	 * for static mesh vehicles, that do not have anim blueprints like skeletal do.
	 */
	void SetAnimatorClass(TSubclassOf<UStaticArcadeVehicleAnimator> NewAnimatorClass);

	/**
	 * Allows to set static animator class for this component.
	 * This class is used to animate vehicle suspension and wheels
	 * for static mesh vehicles, that do not have anim blueprints like skeletal do.
	 * @param NewAnimatorClass Class of the animator we want to set.
	 * @returns True if successful, false if the animator class is invalid.
	 */
	UFUNCTION(BlueprintCallable, Category = Animation)
	bool SetVehicleAnimatorClass(TSubclassOf<UStaticArcadeVehicleAnimator> NewAnimatorClass);	
	
protected:
	/**
	 * Class of the static vehicle movement animator.
	 * Animation is created at runtime at the moment of initialization,
	 * and provides suspension and wheels animation logic.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation)
	TSubclassOf<UStaticArcadeVehicleAnimator> AnimatorClass; 
	
private:
	/**
	* Stores vehicle root primitive
	* component cast to static mesh.
	* This is hidden mesh, but used for the simulation only.
	*/
	UPROPERTY(Transient)
	UStaticMeshComponent* VehiclePhysicsMesh;

	/**
	* Stores actual vehicle mesh. It should be a child of
	* the physical component. This component should be visible,
	* but do not have physics at all.
	* They are separate beings in order to allow decoupling them
	* to simulate tilt and roll animations.
	*/
	UPROPERTY(Transient)
	UStaticMeshComponent* VehicleVisualMesh;

	/** Reference to the current animator instance created for this component. */
	UPROPERTY(Transient)
	UStaticArcadeVehicleAnimator* VehicleAnimatorInstance;
};
