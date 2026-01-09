/** Created and owned by Furious Production LTD @ 2023. **/

#pragma once
#include "GameFramework/Pawn.h"
#include "ArcadeVehiclePawn.generated.h"

class UArcadeVehicleMovementComponent;
class USkeletalMeshComponent;

/**
  * Base class of the arcade vehicle. Because I want to keep this plugin as flexible as possible,
  * the whole vehicle movement is done entirely inside the single component,
  * so you may or may not use this pawn. You can absolutely create your own pawn without
  * deriving from this class. However, it's common to use this class as base, because it implements tiny bits
  * so that the vehicle is ready to go.
  * This pawn is designed for the vehicles using Skeletal Mesh Component!
*/
UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="SkeletalArcadeVehiclePawn"))
class ARCADEVEHICLESYSTEM_API AArcadeVehiclePawn : public APawn
{
	GENERATED_BODY()

public:
	AArcadeVehiclePawn();

	/** Enable and disable the vehicles. */
	UFUNCTION(BlueprintImplementableEvent, Category = Events)
	void SetVehicleEnabled(bool Enable);

protected:
	/** Skeletal mesh of this vehicle. It acts as root component. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* VehicleMesh;

	/** Component responsible for the whole movement and networking physics. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Movement, meta=(AllowPrivateAccess = "true"))
	UArcadeVehicleMovementComponent* MovementComponent;
};