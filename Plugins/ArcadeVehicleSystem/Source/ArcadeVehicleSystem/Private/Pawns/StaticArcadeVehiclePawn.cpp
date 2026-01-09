/** Created and owned by Furious Production LTD @ 2023. **/

#include "Pawns/StaticArcadeVehiclePawn.h"
#include "Movement/StaticArcadeVehicleMovementComponent.h"
#include "Components/StaticMeshComponent.h"

AStaticArcadeVehiclePawn::AStaticArcadeVehiclePawn()
{
	/* Setup basic components. */
	VehicleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VehicleMesh"));
	SetRootComponent(VehicleMesh);
	VehicleVisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VehicleVisualMesh"));
	VehicleVisualMesh->SetupAttachment(VehicleMesh);
	MovementComponent = CreateDefaultSubobject<UStaticArcadeVehicleMovementComponent>(TEXT("MovementComponent"));	

	/* Allow replication, but not the movement, as we are doing this ourselves. */
	bReplicates = true;
	SetReplicateMovement(false);
}
