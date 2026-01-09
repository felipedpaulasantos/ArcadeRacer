// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pawns/StaticArcadeVehiclePawn.h"
#include "StaticVehicleExample.generated.h"

class UStaticArcadeVehicleAnimator;

/**
 * This is example vehicle class implementation in pure C++ without blueprint usage.
 * It shows how to create vehicles without blueprints and how to set them up so that they work
 * completely programatically.
 * This is example for using static mesh vehicle, however, skeletal meshes work in much the same way.
 */
UCLASS(BlueprintType, Blueprintable)
class ARCADEVEHICLESYSTEM_API AStaticVehicleExample : public AStaticArcadeVehiclePawn
{
	GENERATED_BODY()

public:
	AStaticVehicleExample();
	
	/** APawn */
	void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	/** ~APawn */

	/** Initializes vehicle properties required for the vehicle to run. */
	void InitializeVehicleProperties();
};
