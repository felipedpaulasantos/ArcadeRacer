/** Created and owned by Furious Production LTD @ 2023. **/

#include "Movement/StaticArcadeVehicleMovementComponent.h"
#include "Animations/StaticArcadeVehicleAnimator.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshSocket.h"

UStaticArcadeVehicleMovementComponent::UStaticArcadeVehicleMovementComponent()
{
	AnimatorClass = UStaticArcadeVehicleAnimator::StaticClass();
}

bool UStaticArcadeVehicleMovementComponent::InitializeVehicleMovement()
{
	/* Everything must've been registered successfully in the parent class. */
	if(!Super::InitializeVehicleMovement())
	{
		return false;
	}

	/* Check if we have valid animator class. */
	if(!IsValid(AnimatorClass))
	{
		return false;
	}

	/* Initialize static mesh component reference for the physics. */
	VehiclePhysicsMesh = Cast<UStaticMeshComponent>(PhysicsPrimitive);

	/* Validity of this operation define success of the initialization. */
	if(!IsValid(VehiclePhysicsMesh))
	{
		UE_LOG(LogArcadeVehicleMovement, Error, TEXT("Arcade vehicle uses static mesh movement component, but the root is not static mesh!"));
		return false;
	}

	/* Initialize child static mesh for the vehicle visuals. */
	VehicleVisualMesh = Cast<UStaticMeshComponent>(PhysicsPrimitive->GetChildComponent(0));

	/* Check if we have child for visualization. */
	if(!IsValid(VehicleVisualMesh))
	{
		UE_LOG(LogArcadeVehicleMovement, Error, TEXT("Arcade vehicle uses static mesh movement component, but the root needs child static mesh for vehicle!"));
		return false;
	}

	/* Create animator component, but only if it's invalid, we don't want to create it twice. */
	if(!IsValid(VehicleAnimatorInstance))
	{
		/* Check if potentially the animator component already exists(legacy). */
		UStaticArcadeVehicleAnimator* pExistingAnimator = GetOwner()->FindComponentByClass<UStaticArcadeVehicleAnimator>();
		if(IsValid(pExistingAnimator))
		{
			pExistingAnimator->DestroyComponent();
		}
		VehicleAnimatorInstance = NewObject<UStaticArcadeVehicleAnimator>(GetOwner(), AnimatorClass);
		VehicleAnimatorInstance->RegisterComponent();
	}
	
	/* All good! */
	return true;
}

bool UStaticArcadeVehicleMovementComponent::RegisterSuspensionSprings()
{
	/* Iterate over all sockets of the static mesh to register suspension locations. */
	for (FVehicleSuspensionSpring& spring : Settings.Suspension.Springs)
	{
		/* Cache iterated spring socket name. */
		const FName& socketName = spring.BoneName;

		/* Validate socket pointer. */
		const UStaticMeshSocket* pSocket = VehiclePhysicsMesh->GetSocketByName(socketName);
		if (IsValid(pSocket))
		{
			/* Grab socket location and apply it to the iterated spring. */
			spring.Location = pSocket->RelativeLocation;			
		}
		else
		{
			UE_LOG(LogArcadeVehicleMovement, Error, TEXT("Suspension socket %s was not found in the static mesh!"), *socketName.ToString());
		}
	}
	
	/* Implemented and successful. */
	return true;
}

void UStaticArcadeVehicleMovementComponent::GetWheelsBaseTransform(FTransform& OutTransform) const
{
	/* Grab the relative location from the visuals mesh. */
	OutTransform = VehicleVisualMesh->GetComponentTransform();
}

UStaticMeshComponent* UStaticArcadeVehicleMovementComponent::GetVisualsMesh()
{
	return VehicleVisualMesh;
}

void UStaticArcadeVehicleMovementComponent::SetAnimatorClass(
	TSubclassOf<UStaticArcadeVehicleAnimator> NewAnimatorClass)
{
	AnimatorClass = NewAnimatorClass;

	/* If we already have valid animator, we need to replace it. */
	if(IsValid(VehicleAnimatorInstance))
	{
		VehicleAnimatorInstance->DestroyComponent();
		VehicleAnimatorInstance = NewObject<UStaticArcadeVehicleAnimator>(GetOwner(), AnimatorClass);
		VehicleAnimatorInstance->RegisterComponent();
	}
}

bool UStaticArcadeVehicleMovementComponent::SetVehicleAnimatorClass(
	TSubclassOf<UStaticArcadeVehicleAnimator> NewAnimatorClass)
{
	/* Validate the class. */
	if(!IsValid(NewAnimatorClass))
	{
		UE_LOG(LogArcadeVehicleMovement, Error, TEXT("SetVehicleAnimatorClass failed, because the provided animation class is invalid!"));
		return false;
	}
	
	/* Update the class. */
	AnimatorClass = NewAnimatorClass;

	/* Destroy the current animator if it exists. */
	if(IsValid(VehicleAnimatorInstance))
	{
		VehicleAnimatorInstance->DestroyComponent();
		VehicleAnimatorInstance = nullptr;
	}

	/* Create a new animation instance. */
	VehicleAnimatorInstance = NewObject<UStaticArcadeVehicleAnimator>(GetOwner(), AnimatorClass);
	VehicleAnimatorInstance->RegisterComponent();

	/* Success. */
	return true;
}