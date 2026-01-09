/** Created and owned by Furious Production LTD @ 2023. **/

#pragma once
#include "CoreMinimal.h"
#include "ArcadeVehicleNetSerialization.h"
#include "ArcadeVehicleNetworkHelpers.generated.h"

/**
	Gathers some of the initial binary inputs for the vehicle networking.
*/
enum EVehicleInputFlags : uint8
{
	AVS_IF_IsDrifting = 1,
	AVS_IF_IsStabilizing = 2
};

/**
 * Enum containing various vehicle flags, that are replicated.
 * Useful for sharing various data about temporary blocking acceleration etc.
 */
enum EVehicleMovementModifier : uint8
{
	AVS_MM_BlockAcceleration = 1
};

/**
	Input state sent from the controlling client to the server, when using server authoritative vehicles.
	It not only contains inputs, but also contains frame number, so that the controlling client knows which state
	this input is feedback for.
*/
USTRUCT(BlueprintType)
struct ARCADEVEHICLESYSTEM_API FVehicleInputState
{
	GENERATED_BODY()

	FVehicleInputState();

	/** Helpers for managing bitmask inputs. */
	void SetInternalBitflagInput(uint8 Input, bool Set);
	bool CheckInternalBitflagInput(uint8 Input) const;
	void SetCustomBitflagInput(uint8 Input, bool Set);
	bool CheckCustomBitflagInput(uint8 Input) const;
	void SetIsDrifting(bool Set);
	bool IsDrifting() const;
	void SetIsStabilizing(bool Set);
	bool IsStabilizing() const;

	static FVehicleInputState Lerp(const FVehicleInputState& A, const FVehicleInputState& B, float Alpha);
	
	/** Acceleration input provided by this state. */
	UPROPERTY(BlueprintReadOnly, Category=Input)
	FFloat_NetQuantize AccelerationInput;

	/** Turning input provided by this state. */
	UPROPERTY(BlueprintReadOnly, Category=Input)
	FFloat_NetQuantize TurningInput;

	/** Additional float input, optional, for user to utilize. */
	UPROPERTY(BlueprintReadOnly, Category=Input)
	FFloat_NetQuantize CustomInput;

	/** Additional bitflags. Optional, for user to utilize. */
	UPROPERTY(BlueprintReadOnly, Category=Input)
	uint8 CustomBitflags;

	/** Internal bitflags for internal inputs. */
	UPROPERTY()
	uint8 InternalBitflags;
};

/**
	Grouped vehicle runtime information gathered
	using the vehicle physics simulation.
	The values in this state are ordered in ascending order in terms of memory
	to make it CPU cache-perfect, to waste the least possible memory.
*/
USTRUCT()
struct ARCADEVEHICLESYSTEM_API FVehiclePhysicsState
{
	GENERATED_BODY()

	FVehiclePhysicsState();

	static FVehiclePhysicsState Lerp(const FVehiclePhysicsState& A, const FVehiclePhysicsState& B, float Alpha);

	/** Movement modifiers handling. */
	template<typename TMovementModifierType>
	bool CheckMovementModifier(TMovementModifierType Modifier) const
	{
		return MovementModifiers & static_cast<uint8>(Modifier);
	}
	template<typename TMovementModifierType>
	void SetMovementModifier(TMovementModifierType Modifier, bool bSet)
	{
		const uint8 intModifier = static_cast<uint8>(Modifier);
		if(bSet)
		{
			MovementModifiers |= intModifier;
		}
		else
		{
			MovementModifiers &= ~intModifier;
		}
	}
	uint8 GetMovementModifiers() const;
	void SetMovementModifiers(uint8 Modifiers);
	
	/** Server time stamp of this state. It defines how much time ago this state was calculated on its authoritative side. */
	UPROPERTY()
	float TimeStamp;

	/** Input that was used to generate this state. */
	UPROPERTY()
	FVehicleInputState Input;
	
	/** Location of this vehicle at this state. */
	UPROPERTY()
	FVector_NetQuantize Location;

	/** Rotation of this vehicle at this state. */
	UPROPERTY()
	FRotator Rotation;

	/** Linear velocity at this state. */
	UPROPERTY()
	FVector_NetQuantize LinearVelocity;

	/** Angular velocity at this state. */
	UPROPERTY()
	FVector_NetQuantize AngularVelocity;

private:
	/** Vehicle flags containing arbitrary movement modifiers. */
	UPROPERTY()
	uint8 MovementModifiers;
};

/** Special type of array that is completely static and replaces physics states at the bottom if its size is exceeded. */
struct ARCADEVEHICLESYSTEM_API FVehiclePhysicsStateArray
{
	FVehiclePhysicsStateArray();
	FVehiclePhysicsStateArray(int32 InMaxBufferSize);

	/** Returns current number of elements in this array. */
	uint8 Num() const;

	/** Adds new state at the end of this array. If it exceeds size of this array it will pop the oldest state. */
	void AddState(const FVehiclePhysicsState& State);

	/** Takes the first state from the states array. All other states go down in the hierarchy. Returns false if there is no state. */
	bool PullState(FVehiclePhysicsState& OutState);

	/** Returns most suitable state at given time. It uses advanced lerping for getting the best possible state. */
	bool GetSuitableState(float InTime, FVehiclePhysicsState& OutState) const;
	
	/** Returns first state from buffer. */
	FVehiclePhysicsState FirstState() const;

	/** Returns last state from buffer. */
	FVehiclePhysicsState LastState() const;

	/** Clears this buffer. */
	void Clear();

	/** Removes states from buffer that are older than given time. */
	void ClearOldStates(float InTime);

private:
	/** Max size of the buffer. */
	int32 MaxBufferSize;
	
	/** Buffer of states. */
	TArray<FVehiclePhysicsState> Buffer;
};

/**
 * Class storing network correction data.
 */
template<typename T>
struct TErrorCorrectionData
{
	void Reset()
	{
		ErrorValue = T();
		ValueOfCorrection = T();
		bIsCorrecting = false;
	}

	T ErrorValue = T();
	T ValueOfCorrection = T();
	bool bIsCorrecting = false;
};
using VectorCorrectionData = TErrorCorrectionData<FVector>; 
using QuatCorrectionData = TErrorCorrectionData<FQuat>;

/** Structure storing various arcade vehicle forces. */
struct FVehicleForces
{
	FVehicleForces();
	
	float Braking;
	float EngineBraking;
	float Acceleration;
	float Turning;

	float LastAppliedBraking;
	float LastAppliedAcceleration;
};

/** Structure storing various arcade vehicle physics properties at runtime. */
struct FVehiclePhysicsRuntime
{
	FVehiclePhysicsRuntime();

	/** Movement modifiers handling. */
	template<typename TMovementModifierType>
	bool CheckMovementModifier(TMovementModifierType Modifier) const
	{
		return MovementModifiers & static_cast<uint8>(Modifier);
	}
	template<typename TMovementModifierType>
	void SetMovementModifier(TMovementModifierType Modifier, bool bSet)
	{
		const uint8 intModifier = static_cast<uint8>(Modifier);
		if(bSet)
		{
			MovementModifiers |= intModifier;
		}
		else
		{
			MovementModifiers &= ~intModifier;
		}
	}
	
	FVector TotalFrictionSnapLocation;
	bool bHasLastTotalFriction;
	FVector LocalLinearVelocity;
	FVector AngularVelocity;
	float CurrentSpeed;
	float CurrentSpeedUnit;
	bool bIsBraking;
	bool bIsEngineBraking;
	bool bIsAccelerating;
	bool bIsDrifting;
	float AdherenceMultiplier;
	float RotationMultiplier;
	uint8 MovementModifiers;
};