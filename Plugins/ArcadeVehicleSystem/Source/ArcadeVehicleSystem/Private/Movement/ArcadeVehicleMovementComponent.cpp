/** Created and owned by Furious Production LTD @ 2023. **/

#include "Movement/ArcadeVehicleMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"

bool UArcadeVehicleMovementComponent::InitializeVehicleMovement()
{
	/* Everything must've been registered successfully in the parent class. */
	if(!Super::InitializeVehicleMovement())
	{
		return false;
	}

	/* Initialize skeletal mesh component reference. */
	m_pVehicleSkeletalMesh = Cast<USkeletalMeshComponent>(PhysicsPrimitive);

	/* Validity of this operation define success of the initialization. */
	if(!IsValid(m_pVehicleSkeletalMesh))
	{
		UE_LOG(LogArcadeVehicleMovement, Error, TEXT("Arcade vehicle uses skeletal mesh movement component, but the root is not skeletal mesh!"));
		return false;
	}

	/* All good! */
	return true;
}

bool UArcadeVehicleMovementComponent::RegisterSuspensionSprings()
{
	/* Iterate over all bones to register suspension locations. */
	for (FVehicleSuspensionSpring& spring : Settings.Suspension.Springs)
	{
		/* Cache iterated spring bone name. */
		const FName& boneName = spring.BoneName;

		/* Validate bone by index. */
		const int32 boneIndex = m_pVehicleSkeletalMesh->GetBoneIndex(boneName);
		if (boneIndex != INDEX_NONE)
		{
			/* Grab bone location and apply it to the iterated spring. */
			spring.Location = m_pVehicleSkeletalMesh->GetBoneLocation(boneName, EBoneSpaces::ComponentSpace);			
		}
		else
		{
			UE_LOG(LogArcadeVehicleMovement, Error, TEXT("Suspension bone %s was not found in the skeleton!"), *boneName.ToString());
		}
	}

	/* Register parent bone if valid. */
	if (Settings.Suspension.SuspensionParentBoneName.IsValid())
	{
		Settings.Suspension.SuspensionParentBoneIndex = m_pVehicleSkeletalMesh->GetBoneIndex(Settings.Suspension.SuspensionParentBoneName);
	}

	/* Call warning if not found, but not error. */
	if (Settings.Suspension.SuspensionParentBoneIndex == INDEX_NONE)
	{
		UE_LOG(LogArcadeVehicleMovement, Warning, TEXT("Suspension parent bone not specified or not found. Suspension bones will be transformed by the vehicle root."));
	}

	/* Implemented and successful. */
	return true;
}

void UArcadeVehicleMovementComponent::GetWheelsBaseTransform(FTransform& OutTransform) const
{
	if (Settings.Suspension.SuspensionParentBoneIndex > INDEX_NONE)
	{
		OutTransform = m_pVehicleSkeletalMesh->GetBoneTransform(Settings.Suspension.SuspensionParentBoneIndex);
	}
	else
	{
		Super::GetWheelsBaseTransform(OutTransform);
	}
}
