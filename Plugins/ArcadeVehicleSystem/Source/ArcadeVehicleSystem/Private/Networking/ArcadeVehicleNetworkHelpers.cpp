/** Created and owned by Furious Production LTD @ 2023. **/

#include "Networking/ArcadeVehicleNetworkHelpers.h"

FVehiclePhysicsState::FVehiclePhysicsState()
	: TimeStamp(0.f)
	, Location(FVector_NetQuantize::ZeroVector)
	, Rotation(FRotator::ZeroRotator)
	, LinearVelocity(FVector_NetQuantize::ZeroVector)
	, AngularVelocity(FVector_NetQuantize::ZeroVector)
	, MovementModifiers(0)
{
}

FVehiclePhysicsState FVehiclePhysicsState::Lerp(const FVehiclePhysicsState& A, const FVehiclePhysicsState& B, float Alpha)
{
	FVehiclePhysicsState lerpState;
	lerpState.Input = FVehicleInputState::Lerp(A.Input, B.Input, Alpha);
	lerpState.Location = FMath::Lerp(A.Location, B.Location, Alpha);
	lerpState.Rotation = FQuat::Slerp(A.Rotation.Quaternion(), B.Rotation.Quaternion(), Alpha).Rotator();
	lerpState.LinearVelocity = FMath::Lerp(A.LinearVelocity, B.LinearVelocity, Alpha);
	lerpState.AngularVelocity = FMath::Lerp(A.AngularVelocity, B.AngularVelocity, Alpha);
	if(Alpha < 0.5f)
	{
		lerpState.MovementModifiers = A.MovementModifiers;
	}
	else
	{
		lerpState.MovementModifiers = B.MovementModifiers;
	}
	return lerpState;
}

uint8 FVehiclePhysicsState::GetMovementModifiers() const
{
	return MovementModifiers;
}

void FVehiclePhysicsState::SetMovementModifiers(uint8 Modifiers)
{
	MovementModifiers = Modifiers;
}

FVehiclePhysicsStateArray::FVehiclePhysicsStateArray()
	: MaxBufferSize(0)
{
}

FVehiclePhysicsStateArray::FVehiclePhysicsStateArray(int32 InMaxBufferSize)
	: MaxBufferSize(InMaxBufferSize)
{
	Buffer.Reserve(MaxBufferSize);
}

uint8 FVehiclePhysicsStateArray::Num() const
{
	return Buffer.Num();
}

void FVehiclePhysicsStateArray::AddState(const FVehiclePhysicsState& State)
{
	/* Check if the array is full. */
	if (Num() >= MaxBufferSize)
	{
		/* Remove oldest states so that we reach max - 1 size. */
		const int32 overflowCount = (Num() - MaxBufferSize) + 1;
#if UE_5_6_OR_LATER
		Buffer.RemoveAt(0, overflowCount, EAllowShrinking::No);
#else
		Buffer.RemoveAt(0, overflowCount, false);
#endif
	}

	/* Simply add to the list. */
	Buffer.Add(State);
}

bool FVehiclePhysicsStateArray::PullState(FVehiclePhysicsState& OutState)
{
	/* If buffer is empty, bail with false. */
	if (Num() == 0)
	{
		return false;
	}

	OutState = Buffer[0];
#if UE_5_6_OR_LATER
	Buffer.RemoveAt(0, 1, EAllowShrinking::No);
#else
	Buffer.RemoveAt(0, 1, false);
#endif	

	/* Return with success. */
	return true;
}

bool FVehiclePhysicsStateArray::GetSuitableState(float InTime, FVehiclePhysicsState& OutState) const
{
	/* Indexes of begin and end state. */
	int32 beginStateIndex = INDEX_NONE;
	int32 endStateIndex = INDEX_NONE;
	
	/* Run through all states. */
	for(int32 i = 0; i < Buffer.Num(); ++i)
	{
		const FVehiclePhysicsState& state = Buffer[i];
		
		/* If state is later than given time. */
		if(state.TimeStamp > InTime)
		{
			/* This is our end state. */
			endStateIndex = i;
			break;
		}
	}

	/* Figure out if we have begin state. */
	beginStateIndex = endStateIndex - 1;

	/* If we have both states. */
	if(beginStateIndex > INDEX_NONE && endStateIndex > INDEX_NONE)
	{
		/* Interpolate states. */
		const FVehiclePhysicsState& beginState = Buffer[beginStateIndex];
		const FVehiclePhysicsState& endState = Buffer[endStateIndex];
		const float ab = endState.TimeStamp - beginState.TimeStamp;
		const float tb = endState.TimeStamp - InTime;
		const float alpha = tb / ab;
		OutState = FVehiclePhysicsState::Lerp(Buffer[beginStateIndex], Buffer[endStateIndex], alpha);
		return true;
	}
	
	/* If we only have end state. */
	if(endStateIndex > INDEX_NONE)
	{
		/* It's our best option. */
		OutState = Buffer[endStateIndex];
		return true;
	}

	/* We don't have anything newer than this data, so we can take the latest known state. */
	if(Buffer.Num() > 0)
	{
		OutState = Buffer.Last();
		return true;
	}

	/* We have shit. */
	return false;
}

FVehiclePhysicsState FVehiclePhysicsStateArray::FirstState() const
{
	return Num() > 0 ? Buffer[0] : FVehiclePhysicsState();
}

FVehiclePhysicsState FVehiclePhysicsStateArray::LastState() const
{
	return Num() > 0 ? Buffer.Last() : FVehiclePhysicsState();
}

void FVehiclePhysicsStateArray::Clear()
{
	Buffer.Reset();
}

void FVehiclePhysicsStateArray::ClearOldStates(float InTime)
{
	/* Run through all states. */
	for(int32 i = Buffer.Num() - 1; i >= 0; --i)
	{
		const FVehiclePhysicsState& state = Buffer[i];
		
		/* If state is older than given time. */
		if(state.TimeStamp < InTime)
		{
			/* We remove from here to the beginning. */
			const int32 countToRemove = FMath::Max(1, i - 1);
#if UE_5_6_OR_LATER
			Buffer.RemoveAt(0, countToRemove, EAllowShrinking::No);
#else
			Buffer.RemoveAt(0, countToRemove, false);
#endif			
			break;
		}
	}
}

FVehicleForces::FVehicleForces()
	: Braking(0.f)
	, EngineBraking(0.f)
	, Acceleration(0.f)
	, Turning(0.f)
	, LastAppliedBraking(0.f)
	, LastAppliedAcceleration(0.f)
{
}

FVehiclePhysicsRuntime::FVehiclePhysicsRuntime()
	: TotalFrictionSnapLocation(FVector::ZeroVector)
	, bHasLastTotalFriction(false)
	, LocalLinearVelocity(FVector::ZeroVector)
	, AngularVelocity(FVector::ZeroVector)
	, CurrentSpeed(0.f)
	, CurrentSpeedUnit(0.f)
	, bIsBraking(false)
	, bIsEngineBraking(false)
	, bIsAccelerating(false)
	, bIsDrifting(false)
	, AdherenceMultiplier(1.f)
	, RotationMultiplier(1.f)
	, MovementModifiers(0)
{
}

FVehicleInputState::FVehicleInputState()
	: AccelerationInput(0.f)
	, TurningInput(0.f)
	, CustomInput(0.f)
	, CustomBitflags(0)
	, InternalBitflags(0)
{
}

void FVehicleInputState::SetInternalBitflagInput(uint8 Input, bool Set)
{
	if (Set)
	{
		InternalBitflags |= Input;
	}
	else
	{
		InternalBitflags &= ~Input;
	}
}

bool FVehicleInputState::CheckInternalBitflagInput(uint8 Input) const
{
	return InternalBitflags & Input;
}

void FVehicleInputState::SetCustomBitflagInput(uint8 Input, bool Set)
{
	if (Set)
	{
		CustomBitflags |= Input;
	}
	else
	{
		CustomBitflags &= ~Input;
	}
}

bool FVehicleInputState::CheckCustomBitflagInput(uint8 Input) const
{
	return CustomBitflags & Input;
}

void FVehicleInputState::SetIsDrifting(bool Set)
{
	SetInternalBitflagInput(AVS_IF_IsDrifting, Set);
}

bool FVehicleInputState::IsDrifting() const
{
	return CheckInternalBitflagInput(AVS_IF_IsDrifting);
}

void FVehicleInputState::SetIsStabilizing(bool Set)
{
	SetInternalBitflagInput(AVS_IF_IsStabilizing, Set);
}

bool FVehicleInputState::IsStabilizing() const
{
	return CheckInternalBitflagInput(AVS_IF_IsStabilizing);
}

FVehicleInputState FVehicleInputState::Lerp(const FVehicleInputState& A, const FVehicleInputState& B, float Alpha)
{
	FVehicleInputState lerpState;
	lerpState.AccelerationInput = FMath::Lerp(A.AccelerationInput.ToFloat(), B.AccelerationInput.ToFloat(), Alpha);
	lerpState.TurningInput = FMath::Lerp(A.TurningInput.ToFloat(), B.TurningInput.ToFloat(), Alpha);
	lerpState.CustomInput = FMath::Lerp(A.CustomInput.ToFloat(), B.CustomInput.ToFloat(), Alpha);
	if(Alpha < 0.5f)
	{
		lerpState.InternalBitflags = A.InternalBitflags;
		lerpState.CustomBitflags = A.CustomBitflags;
	}
	else
	{
		lerpState.InternalBitflags = B.InternalBitflags;
		lerpState.CustomBitflags = B.CustomBitflags;
	}
	return lerpState;
}
