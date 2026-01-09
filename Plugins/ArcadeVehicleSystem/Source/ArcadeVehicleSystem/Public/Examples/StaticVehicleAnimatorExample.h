// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animations/StaticArcadeVehicleAnimator.h"
#include "StaticVehicleAnimatorExample.generated.h"


/**
 * Example C++ class for the static arcade vehicle animations.
 * It's only example showing how to set up animation settings in pure C++;
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ARCADEVEHICLESYSTEM_API UStaticVehicleAnimatorExample : public UStaticArcadeVehicleAnimator
{
	GENERATED_BODY()

public:
	UStaticVehicleAnimatorExample();

	/** Initializes default animator values. */
	void InitializeAnimatorSettings();
};
