/** Created and owned by Furious Production LTD @ 2023. **/

#pragma once
#include "CoreMinimal.h"
#include "ArcadeVehicleMovementComponentBase.h"
#include "ArcadeVehicleMovementComponent.generated.h"

class USkeletalMeshComponent;

/**
 * Implements arcade vehicle movement component logic for
 * the vehicles that are created using Skeletal Mesh Component.
 * If your vehicles use Static Mesh Component, please use different movement
 * component for it.
 */
UCLASS(BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent, DisplayName="SkeletalArcadeVehicleMovementComponent"))
class ARCADEVEHICLESYSTEM_API UArcadeVehicleMovementComponent : public UArcadeVehicleMovementComponentBase
{
	GENERATED_BODY()

public:
	/** UArcadeVehicleMovementComponentBase interface. */
	bool InitializeVehicleMovement() override;
	bool RegisterSuspensionSprings() override;
	void GetWheelsBaseTransform(FTransform& OutTransform) const override;
	/** ~UArcadeVehicleMovementComponentBase interface. */
	
private:
	/**
	 * Stores vehicle root primitive
	 * component cast to skeletal mesh.
	 */
	UPROPERTY()
	USkeletalMeshComponent* m_pVehicleSkeletalMesh;
};
