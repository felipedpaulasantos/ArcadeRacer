/** Created and owned by Furious Production LTD @ 2023. **/

#pragma once
#include "GameFramework/Pawn.h"
#include "StaticArcadeVehiclePawn.generated.h"

class UStaticArcadeVehicleMovementComponent;
class UStaticMeshComponent;

/**
* Base class of the arcade vehicle. Because I want to keep this plugin as flexible as possible,
* the whole vehicle movement is done entirely inside the single component,
* so you may or may not use this pawn. You can absolutely create your own pawn without
* deriving from this class. However, it's common to use this class as base, because it implements tiny bits
* so that the vehicle is ready to go.
* This pawn is designed for the vehicles using Static Mesh Component!
*/
UCLASS(BlueprintType, Blueprintable)
class ARCADEVEHICLESYSTEM_API AStaticArcadeVehiclePawn : public APawn
{
	GENERATED_BODY()

	public:
	AStaticArcadeVehiclePawn();

	/** Enable and disable the vehicles. */
	UFUNCTION(BlueprintImplementableEvent, Category = Events)
	void SetVehicleEnabled(bool Enable);

	protected:
	/** Static mesh of this vehicle. It acts as root component and simulates all vehicle physics. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* VehicleMesh;

	/** Static mesh of this vehicle used purely for visual aspects of it without collision or physics. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* VehicleVisualMesh;

	/** Component responsible for the whole movement and networking physics. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Movement, meta=(AllowPrivateAccess = "true"))
	UStaticArcadeVehicleMovementComponent* MovementComponent;
};