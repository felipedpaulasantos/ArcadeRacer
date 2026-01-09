/** Created and owned by Furious Production LTD @ 2023. **/

#include "Movement/ArcadeVehicleMovementComponentBase.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "Curves/CurveFloat.h"
#include "Movement/ArcadeVehiclePathFollowingComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"

DEFINE_LOG_CATEGORY(LogArcadeVehicleMovement);

UArcadeVehicleMovementComponentBase::UArcadeVehicleMovementComponentBase()
{
	/* Allow replication. */
	SetIsReplicatedByDefault(true);

	/* Setup ticking rules. */
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	//
	PrePhysicsTick.TickGroup = TG_PrePhysics;
	PrePhysicsTick.bCanEverTick = true;
	PrePhysicsTick.bStartWithTickEnabled = false;
	PrePhysicsTick.SetTickFunctionEnable(false);
	//
	PostPhysicsTick.TickGroup = TG_PostPhysics;
	PostPhysicsTick.bCanEverTick = true;
	PostPhysicsTick.bStartWithTickEnabled = false;
	PostPhysicsTick.SetTickFunctionEnable(false);

	/* External data. */
	SetAccelerationInput(0.f);
	SetTurningInput(0.f);
	SetDriftInput(false);
	SetStabilizationInput(false);
	MaxSpeedMultiplier = 1.f;

	/* Internal data. */
	bIsVehicleInitialized = false;
	LastTeleportTime = 0.f;
	CustomGravity = FVector::ZeroVector;
}

void UArcadeVehicleMovementComponentBase::BeginPlay()
{
	Super::BeginPlay();
	
	/* Apply vehicle settings in full. */
	ApplyVehicleSettings();
}

void UArcadeVehicleMovementComponentBase::RegisterComponentTickFunctions(bool bRegister)
{
	Super::RegisterComponentTickFunctions(bRegister);

	/* Register movement v2 ticking functions. */
	if(bRegister)
	{
		if(SetupActorComponentTickFunction(&PrePhysicsTick))
		{
			PrePhysicsTick.Target = this;
		}
		if(SetupActorComponentTickFunction(&PostPhysicsTick))
		{
			PostPhysicsTick.Target = this;
		}
	}
	else
	{
		if(PrePhysicsTick.IsTickFunctionRegistered())
		{
			PrePhysicsTick.UnRegisterTickFunction();
		}
		if(PostPhysicsTick.IsTickFunctionRegistered())
		{
			PostPhysicsTick.UnRegisterTickFunction();
		}
	}
}

void UArcadeVehicleMovementComponentBase::SetComponentTickEnabled(bool bEnabled)
{
	/* Override enabled flag. */
	bEnabled = bEnabled && bIsVehicleInitialized;

	/* Enable in parent class. */
	Super::SetComponentTickEnabled(bEnabled);

	/* Register our custom ticks. */
	if(PrePhysicsTick.bCanEverTick && !IsTemplate())
	{
		PrePhysicsTick.SetTickFunctionEnable(bEnabled);
	}
	if(PostPhysicsTick.bCanEverTick && !IsTemplate())
	{
		PostPhysicsTick.SetTickFunctionEnable(bEnabled);
	}
}

void UArcadeVehicleMovementComponentBase::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	/* Check who controls this vehicle now. */
	AController* pNewController = IsValid(GetPawnOwner()) ? GetPawnOwner()->GetController() : nullptr;
	if(pNewController != CurrentController)
	{
		CurrentController = pNewController;

		/* Clear network data. */
		ClearNetworkData();
	}
	
	/* Skip any sort of physics calculations as soon as the physics is disabled. */
	if (!PhysicsPrimitive->IsSimulatingPhysics())
	{
		return;
	}

	/* Deploy tick accordingly. */
	if(ThisTickFunction == &PrePhysicsTick)
	{
		OnPrePhysicsTick(DeltaTime);
	}
	else if(ThisTickFunction == &PostPhysicsTick)
	{
		OnPostPhysicsTick(DeltaTime);
	}

	/* If locally controlled, always mark for camera updates, so we get net relevancy to work properly. */
	if (GetPawnOwner()->IsLocallyControlled())
	{
		MarkForClientCameraUpdate();
	}
}

void UArcadeVehicleMovementComponentBase::RequestPathMove(const FVector& MoveInput)
{
	/* Prepare acceleration and turning. */
	const float accelDot = FMath::Clamp(
		FVector::DotProduct(PhysicsPrimitive->GetForwardVector(), MoveInput), 0.f, 1.f);
	
	const float turnDot = FMath::Clamp(
		FVector::DotProduct(PhysicsPrimitive->GetRightVector(), MoveInput), -1.f, 1.f);

	/* Apply acceleration and turning inputs. */
	SetAccelerationInput(accelDot);
	SetTurningInput(turnDot);
}

bool UArcadeVehicleMovementComponentBase::IsMovingOnGround() const
{
	const int32 totalWheelsOnGround = WheelsInfo.DriveWheelsOnGround + WheelsInfo.SteeringWheelsOnGround;
	return totalWheelsOnGround > 0;
}

bool UArcadeVehicleMovementComponentBase::IsFalling() const
{
	const int32 totalWheelsOnGround = WheelsInfo.DriveWheelsOnGround + WheelsInfo.SteeringWheelsOnGround;
	return totalWheelsOnGround <= 0;
}

const FVehicleSettings& UArcadeVehicleMovementComponentBase::GetVehicleSettings() const
{
	return Settings;
}

void UArcadeVehicleMovementComponentBase::SetVehicleSettings(const FVehicleSettings& NewSettings)
{
	/* Assign new settings. */
	Settings = NewSettings;

	/* Uninitialize current vehicle physics. ApplyVehicleSettings must be called. */
	bIsVehicleInitialized = false;

	/* Temporarily disable ticking. */
	SetComponentTickEnabled(false);
}

void UArcadeVehicleMovementComponentBase::SetPhysicsSettings(const FVehiclePhysicsSettings& NewSettings)
{
	Settings.Physics = NewSettings;
	SetVehicleSettings(Settings);
}

void UArcadeVehicleMovementComponentBase::SetEngineSettings(const FVehicleEngineSettings& NewSettings)
{
	Settings.Engine = NewSettings;
	SetVehicleSettings(Settings);
}

void UArcadeVehicleMovementComponentBase::SetSteeringSettings(const FVehicleSteeringSettings& NewSettings)
{
	Settings.Steering = NewSettings;
	SetVehicleSettings(Settings);
}

void UArcadeVehicleMovementComponentBase::SetSuspensionSettings(const FVehicleSuspensionSettings& NewSettings)
{
	Settings.Suspension = NewSettings;
	SetVehicleSettings(Settings);
}

void UArcadeVehicleMovementComponentBase::ApplyVehicleSettings()
{
	/* Deinitialize it. */
	bIsVehicleInitialized = false;

	/* Initialize state buffer. */
	StateBuffer = FVehiclePhysicsStateArray(100);
	
	/* Temporarily disable ticking. */
	SetComponentTickEnabled(false);

	/* Register suspension springs if previous initialization was successful. */
	if(InitializeVehicleMovement())
	{
		bIsVehicleInitialized = RegisterSuspensionSprings();
	}
	
	/* Re-enable ticking. It will automatically keep it false if the vehicle is not initialized properly. */
	SetComponentTickEnabled(true);
}

FVector UArcadeVehicleMovementComponentBase::GetVehicleGravity() const
{
	if(Settings.Physics.EnableCustomGravity)
	{
		return CustomGravity;
	}

	return FVector(0.f, 0.f, GetWorld()->GetGravityZ());
}

void UArcadeVehicleMovementComponentBase::SetCustomGravity(const FVector& InGravity)
{
	CustomGravity = InGravity;
}

int32 UArcadeVehicleMovementComponentBase::GetNumberOfDriveWheelsOnGround() const
{
	return WheelsInfo.DriveWheelsOnGround;
}

int32 UArcadeVehicleMovementComponentBase::GetNumberOfSteeringWheelsOnGround() const
{
	return WheelsInfo.SteeringWheelsOnGround;
}

int32 UArcadeVehicleMovementComponentBase::GetNumberOfWheelsOnGround() const
{
	return GetNumberOfDriveWheelsOnGround() + GetNumberOfSteeringWheelsOnGround();
}

float UArcadeVehicleMovementComponentBase::GetTurningInput() const
{
	return CurrentInput.TurningInput.ToFloat();
}

float UArcadeVehicleMovementComponentBase::GetCurrentSpeed() const
{
	return PhysicsRuntime.CurrentSpeed;
}

float UArcadeVehicleMovementComponentBase::GetCurrentSpeedAbsolute() const
{
	return FMath::Abs(GetCurrentSpeed());
}

float UArcadeVehicleMovementComponentBase::GetCurrentSpeedUnit() const
{
	return PhysicsRuntime.CurrentSpeedUnit;
}

FVector UArcadeVehicleMovementComponentBase::GetLocalLinearVelocity() const
{
	return PhysicsRuntime.LocalLinearVelocity;
}

FVector UArcadeVehicleMovementComponentBase::GetLinearVelocityForward() const
{
	const FVector localVelocity(GetLocalLinearVelocity().X, 0.f, 0.f);
	return PhysicsPrimitive->GetComponentTransform().TransformVectorNoScale(localVelocity);
}

FVector UArcadeVehicleMovementComponentBase::GetLinearVelocityRight() const
{
	const FVector localVelocity(0.f, GetLocalLinearVelocity().Y, 0.f);
	return PhysicsPrimitive->GetComponentTransform().TransformVectorNoScale(localVelocity);
}

float UArcadeVehicleMovementComponentBase::GetLastAppliedAcceleration() const
{
	return LastForces.LastAppliedAcceleration;
}

float UArcadeVehicleMovementComponentBase::GetLastAppliedBraking() const
{
	 return LastForces.LastAppliedBraking;
}

void UArcadeVehicleMovementComponentBase::SetMaxSpeedMultiplier(const float NewMaxSpeedMultiplier)
{
	MaxSpeedMultiplier = NewMaxSpeedMultiplier;
}

float UArcadeVehicleMovementComponentBase::GetMaxSpeedMultiplier() const
{
	return MaxSpeedMultiplier;
}

void UArcadeVehicleMovementComponentBase::SetAccelerationInput(const float Value)
{
	LocalInput.AccelerationInput = FMath::Clamp(Value, -1.f, 1.f);
}

void UArcadeVehicleMovementComponentBase::SetTurningInput(const float Value)
{
	LocalInput.TurningInput = FMath::Clamp(Value, -1.f, 1.f);
}

void UArcadeVehicleMovementComponentBase::SetDriftInput(const bool EnableDrift)
{
	LocalInput.SetIsDrifting(EnableDrift);
}

void UArcadeVehicleMovementComponentBase::SetStabilizationInput(const bool EnableStabilization)
{
	LocalInput.SetIsStabilizing(EnableStabilization);
}

void UArcadeVehicleMovementComponentBase::SetCustomInput(float Value)
{
	LocalInput.CustomInput = Value;
}

void UArcadeVehicleMovementComponentBase::SetCustomBitflagInput(uint8 Bitflag, bool bEnableFlag)
{
	LocalInput.SetCustomBitflagInput(Bitflag, bEnableFlag);
}

bool UArcadeVehicleMovementComponentBase::CheckCustomBitflagInput(int32 Bitflag, const FVehicleInputState& Input)
{
	return Input.CheckCustomBitflagInput(Bitflag);
}

bool UArcadeVehicleMovementComponentBase::IsAccelerating() const
{
	return PhysicsRuntime.bIsAccelerating;
}

bool UArcadeVehicleMovementComponentBase::IsMovingForward() const
{
	return GetCurrentSpeed() > Settings.Physics.MovementDirectionTolerance;
}

bool UArcadeVehicleMovementComponentBase::IsMovingBackward() const
{
	return GetCurrentSpeed() < -Settings.Physics.MovementDirectionTolerance;
}

bool UArcadeVehicleMovementComponentBase::IsMovingAtAll() const
{
	return IsMovingForward() || IsMovingBackward();
}

bool UArcadeVehicleMovementComponentBase::IsBraking() const
{
	return PhysicsRuntime.bIsBraking;
}

bool UArcadeVehicleMovementComponentBase::IsEngineBraking() const
{
	return PhysicsRuntime.bIsEngineBraking;
}

bool UArcadeVehicleMovementComponentBase::IsDrifting() const
{
	return PhysicsRuntime.bIsDrifting;
}

void UArcadeVehicleMovementComponentBase::TeleportVehicle(const FTransform& Transform)
{
	/* If we control this vehicle or we are the server already. */
	if(HasControlOverVehicle() || GetOwner()->HasAuthority())
	{
		/* Send teleport request to the server. */
		OnReceiveTeleport_Server(Transform.GetLocation(), Transform.GetRotation().Rotator());
	}
	else
	{
		UE_LOG(LogArcadeVehicleMovement, Error, TEXT("TeleportVehicle method can only be called on controlling side, or on server!"));
	}
}

void UArcadeVehicleMovementComponentBase::SetVehicleLinearVelocity(const FVector& LinearVelocity, const bool WorldSpace /*= false*/)
{
	/* Handle world space. */
	if (WorldSpace)
	{
		PhysicsPrimitive->SetPhysicsLinearVelocity(LinearVelocity);
	}
	/* Handle local space. */
	else
	{
		const FVector velocityWorldSpace = PhysicsPrimitive->GetComponentTransform().TransformVectorNoScale(LinearVelocity);
		PhysicsPrimitive->SetPhysicsLinearVelocity(velocityWorldSpace);
	}
}

float UArcadeVehicleMovementComponentBase::GetWheelOffset(int32 Index) const
{
	if (!Settings.Suspension.Springs.IsValidIndex(Index))
	{
		return 0.f;
	}

	return Settings.Suspension.Springs[Index].WheelOffset;
}

bool UArcadeVehicleMovementComponentBase::IsAccelerationBlocked() const
{
	return PhysicsRuntime.CheckMovementModifier(AVS_MM_BlockAcceleration);
}

void UArcadeVehicleMovementComponentBase::SetBlockAcceleration(bool Block)
{
	/* Only allowed on the client that owns vehicle. */
	if(HasControlOverVehicle())
	{
		PhysicsRuntime.SetMovementModifier(AVS_MM_BlockAcceleration, Block);
	}
	else
	{
		UE_LOG(LogArcadeVehicleMovement, Warning, TEXT("SetBlockAcceleration can only be called on the client that controls this vehicle!"));
	}
}

UPrimitiveComponent* UArcadeVehicleMovementComponentBase::GetVehicleMesh() const
{
	return PhysicsPrimitive;
}

void UArcadeVehicleMovementComponentBase::ClearNetworkData()
{
	/* Clear physics state and inputs. */
	ClearInputs();
	StateBuffer.Clear();
	LocationCorrection.Reset();
	RotationCorrection.Reset();
	LinearVelocityCorrection.Reset();
	AngularVelocityCorrection.Reset();
	PhysicsRuntime.bHasLastTotalFriction = false;
}

void UArcadeVehicleMovementComponentBase::ClearInputs()
{
	SetAccelerationInput(0.f);
	SetTurningInput(0.f);
	SetDriftInput(false);
	SetStabilizationInput(false);
}

const UArcadeVehiclePathFollowingComponent* UArcadeVehicleMovementComponentBase::GetPathFollowingComponent()
{
	if (!IsValid(PathFollowingComponent))
	{
		PathFollowingComponent = Cast<UArcadeVehiclePathFollowingComponent>(GetPathFollowingAgent());
	}

	return PathFollowingComponent;
}

bool UArcadeVehicleMovementComponentBase::HasControlOverVehicle() const
{
	return (GetPawnOwner()->IsPawnControlled() && GetPawnOwner()->IsLocallyControlled()) || (!GetPawnOwner()->IsPawnControlled() && GetPawnOwner()->HasAuthority());
}

void UArcadeVehicleMovementComponentBase::OnPrePhysicsTick(float DeltaTime)
{
	/* Multiply delta time to match simulation speeds of the previous movement curves etc. */
	const float regularDeltaTime = DeltaTime;
	DeltaTime *= 8.f;

	/* Prepare simulation frame. */
	PrepareFrame();

	/* Calculate gravity. */
	CalculateGravity(DeltaTime);
	
	/* Calculate suspension. */
	if(Settings.Advanced.bEnableSuspension)
	{
		CalculateSuspension(regularDeltaTime);
	}

	/* Calculate adherence. */
	float linearAdherence, angularAdherence;
	if(Settings.Advanced.bEnableAdherence)
	{
		CalculateAdherence(DeltaTime, linearAdherence, angularAdherence);
	}
	else
	{
		linearAdherence = Settings.Steering.LinearDamping;
		angularAdherence = Settings.Steering.AngularDamping;
	}

	/* Local-space linear velocity that will be modulated by the acceleration forces. Starts from current one. */
	FVector linearVelocity = PhysicsRuntime.LocalLinearVelocity;
	
	/* Calculate acceleration if some drive wheels touch the ground. */
	if(WheelsInfo.DriveWheelsOnGround > 0)
	{
		if(Settings.Advanced.bEnableAcceleration)
		{
			linearVelocity = CalculateAcceleration(DeltaTime, linearAdherence);
		}
	}

	/* Angular velocity that will be modulated by the turning forces. Start from current one. */
	FVector angularVelocity = PhysicsPrimitive->GetComponentTransform().InverseTransformVectorNoScale(PhysicsRuntime.AngularVelocity);

	/* Calculate steering if some steering wheels are on ground. */
	if(WheelsInfo.SteeringWheelsOnGround > 0)
	{
		if(Settings.Advanced.bEnableAdherence)
		{
			angularVelocity = CalcuateAngularAdherence(DeltaTime, angularAdherence, angularVelocity);
		}
		if(Settings.Advanced.bEnableTurning)
		{
			CalculateTurning(DeltaTime, angularVelocity);
		}
	}

	/* Calculate friction forces. */
	if(Settings.Advanced.bEnableFriction)
	{
		CalculateFriction(DeltaTime, linearVelocity);
	}

	/* Transform final linear velocity from local to world. */
	linearVelocity = PhysicsPrimitive->GetComponentTransform().TransformVectorNoScale(linearVelocity);

	/* Apply this frame linear velocity if at least one drive wheel is on ground. */
	PhysicsPrimitive->SetPhysicsLinearVelocity(linearVelocity);

	/* Apply stabilization if needed. */
	if (CurrentInput.IsStabilizing())
	{
		angularVelocity.X = PhysicsPrimitive->GetComponentRotation().Roll * Settings.Physics.StabilizationForce * DeltaTime;
		angularVelocity.Y = PhysicsPrimitive->GetComponentRotation().Pitch * Settings.Physics.StabilizationForce * DeltaTime;
	}
	
	/* Apply this frame angular velocity. */
	angularVelocity = PhysicsPrimitive->GetComponentTransform().TransformVectorNoScale(angularVelocity);
	PhysicsPrimitive->SetPhysicsAngularVelocityInDegrees(angularVelocity);

	/* Apply custom movement */
	CalculateCustomVehicleMovement.Broadcast(PhysicsPrimitive, CurrentInput, DeltaTime);
}

void UArcadeVehicleMovementComponentBase::OnPostPhysicsTick(float DeltaTime)
{
	/* Fully build state based on current physics information. */
	const FVehiclePhysicsState physicsState = BuildState();

	/* If we are owner of this vehicle. */
	if(HasControlOverVehicle())
	{
		/* Send state to the server. */
		OnReceiveState_Server(physicsState);
	}
	/* If we do not control this vehicle. */
	else
	{
		/* Add state to the buffer. */
		StateBuffer.AddState(physicsState);
	}
}

void UArcadeVehicleMovementComponentBase::PrepareFrame()
{
	/* If we have no control over this vehicle. */
	if(!HasControlOverVehicle())
	{
		/* Apply physics corrections if any. */
		ApplyPhysicsCorrections();

		/* Apply movement modifiers. */
		PhysicsRuntime.MovementModifiers = ServerState.GetMovementModifiers();
	}
	
	/* Gather inputs. */
	GatherInputs();

	/* Prepare forces. */
	CalculateForces();
	
	/* Calculate all states based on the previous frame calculations. */
	CalculatePhysicsRuntimeData();
}

void UArcadeVehicleMovementComponentBase::GatherInputs()
{
	/* If we are controlling this vehicle. */
	if(HasControlOverVehicle())
	{
		/* Current input is our local input. */
		CurrentInput = LocalInput;
	}
	/* If we don't have any control over vehicle. */
	else
	{
		/* Current input comes from oldest state we will be dispatching. */
		CurrentInput = ServerState.Input;
	}
}

void UArcadeVehicleMovementComponentBase::CalculateForces()
{
	/* Cache previous forces in case we need them for calculations. */
	const FVehicleForces previousForces = LastForces;
	
	/* Acceleration and braking forces based on curves and acceleration inputs. */
	LastForces.Braking = Settings.Engine.BrakingCurve->GetFloatValue(GetCurrentSpeedAbsolute());
	LastForces.EngineBraking = Settings.Engine.EngineBrakingCurve->GetFloatValue(GetCurrentSpeedAbsolute());
	LastForces.Acceleration = 0.f;
	if(!PhysicsRuntime.CheckMovementModifier(AVS_MM_BlockAcceleration))
	{
		const float accelerationInput = CurrentInput.AccelerationInput.ToFloat();
		if(accelerationInput > 0.f)
		{
			LastForces.Acceleration = Settings.Engine.AccelerationCurve->GetFloatValue(GetCurrentSpeedAbsolute()) * CurrentInput.AccelerationInput.ToFloat(); 
		}
		else if(accelerationInput < 0.f)
		{
			LastForces.Acceleration = Settings.Engine.ReversingCurve->GetFloatValue(GetCurrentSpeedAbsolute()) * CurrentInput.AccelerationInput.ToFloat(); 
		}
	}

	/* Turning forces based on previous forces, curves and settings. */
	const bool bWasZeroRotation = FMath::IsNearlyZero(previousForces.Turning, 0.2f);

	/* If we have released turning input, but forces are still there, damp steering input slowly to have nice transition. */
	if(CurrentInput.TurningInput == 0.f)
	{
		if(bWasZeroRotation == false)
		{
			LastForces.Turning = FMath::Lerp(0.f, previousForces.Turning, Settings.Steering.SteeringDamping);
		}
	}
	/* If we have some steering input. */
	else
	{
		/* Calculate turning force based on curve. */
		LastForces.Turning = Settings.Steering.SteeringCurve->GetFloatValue(GetCurrentSpeedAbsolute()) * CurrentInput.TurningInput.ToFloat();

		/* Inverse rotation delta if vehicle is moving backwards, because while reversing we should rotate other way. */
		if(IsMovingBackward())
		{
			LastForces.Turning *= -1.f;
		}

		/* If previous rotation was zero, blend new rotation into it slowly, so we don't get hard snap. */
		if(bWasZeroRotation)
		{
			LastForces.Turning = FMath::Lerp(LastForces.Turning, 0.f, Settings.Steering.SteeringDamping);
		}
		/* If we started turning different direction than previously, also blend it. */
		else if(FMath::Sign(CurrentInput.TurningInput.ToFloat()) != FMath::Sign(LastForces.Turning))
		{
			LastForces.Turning = FMath::Lerp(LastForces.Turning, previousForces.Turning, Settings.Steering.SteeringDamping);
		}
		/* If we were and are still turning in the same direction. */
		else
		{
			LastForces.Turning = FMath::Lerp(LastForces.Turning, previousForces.Turning, Settings.Steering.SteeringDamping);
		}
	}
}

bool UArcadeVehicleMovementComponentBase::InitializeVehicleMovement()
{
	/* Find out primitive component from the root of the owner. */
    PhysicsPrimitive = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent());

	/* When we are in sequencer, do not allow initializing the vehicle. */
	if(GetOwner()->ActorHasTag(TEXT("SequencerActor")) && GetVehicleSettings().Advanced.bDisablePhysicsInSequencer)
	{
    	/* Disable physics if possible. */
    	if(IsValid(PhysicsPrimitive))
    	{
    		PhysicsPrimitive->SetSimulatePhysics(false);
    	}

		/* Fail here. */
    	return false;
	}
	
	/* Validate all the data and if not valid, disable ticking immediately. */
	if (!IsValid(PhysicsPrimitive))
	{
		UE_LOG(LogArcadeVehicleMovement, Error, TEXT("Vehicle root component is not UPrimitiveComponent! Use Static or Skeletal mesh for the root!"));
		return false;
	}
	if (!IsValid(Settings.Engine.AccelerationCurve))
	{
		UE_LOG(LogArcadeVehicleMovement, Error, TEXT("Vehicle does not have valid acceleration curve assigned!"));
		return false;
	}
	if (!IsValid(Settings.Engine.ReversingCurve))
	{
		UE_LOG(LogArcadeVehicleMovement, Error, TEXT("Vehicle does not have valid reversing curve assigned!"));
		return false;
	}
	if (!IsValid(Settings.Engine.EngineBrakingCurve))
	{
		UE_LOG(LogArcadeVehicleMovement, Error, TEXT("Vehicle does not have valid engine braking curve assigned!"));
		return false;
	}
	if (!IsValid(Settings.Engine.BrakingCurve))
	{
		UE_LOG(LogArcadeVehicleMovement, Error, TEXT("Vehicle does not have valid braking curve assigned!"));
		return false;
	}
	if (!IsValid(Settings.Steering.SteeringCurve))
	{
		UE_LOG(LogArcadeVehicleMovement, Error, TEXT("Vehicle does not have valid steering curve assigned!"));
		return false;
	}

	/* Disables original gravity when custom is enabled.  */
	PhysicsPrimitive->SetEnableGravity(!Settings.Physics.EnableCustomGravity);

	/* Initially successful. */
	return true;
}

bool UArcadeVehicleMovementComponentBase::RegisterSuspensionSprings()
{
	UE_LOG(LogArcadeVehicleMovement, Error, TEXT("RegisterSuspensionSprings method is not overridden!"));
	return false;
}

void UArcadeVehicleMovementComponentBase::CalculatePhysicsRuntimeData()
{
	/* Resets runtime flags. */
	PhysicsRuntime.bIsBraking = false;
	PhysicsRuntime.bIsEngineBraking = false;
	PhysicsRuntime.bIsAccelerating = false;
	
	/* Calculate global value of the local linear velocity of this vehicle. */
	PhysicsRuntime.LocalLinearVelocity = PhysicsPrimitive->GetComponentTransform().InverseTransformVectorNoScale(PhysicsPrimitive->GetPhysicsLinearVelocity());

	/* Calculate global value of the angular velocity of this vehicle. */
	PhysicsRuntime.AngularVelocity = PhysicsPrimitive->GetPhysicsAngularVelocityInDegrees();

	/* Calculate current speed in km/h. 0.036f is multiplied because of m/s conversion. */
	PhysicsRuntime.CurrentSpeed = PhysicsRuntime.LocalLinearVelocity.X * KMH_MULTIPLIER;

	/* Calculate speed unit depending on direction. */
	PhysicsRuntime.CurrentSpeedUnit = 0.f;
	if (IsMovingForward())
	{
		PhysicsRuntime.CurrentSpeedUnit = FMath::Clamp(GetCurrentSpeedAbsolute() / (Settings.Engine.MaxSpeed * MaxSpeedMultiplier), 0.f, 1.f);
	}
	else if (IsMovingBackward())
	{
		PhysicsRuntime.CurrentSpeedUnit = FMath::Clamp(GetCurrentSpeedAbsolute() / Settings.Engine.MaxReverseSpeed, 0.f, 1.f);
	}

	/* Apply drifting flag. */
	PhysicsRuntime.bIsDrifting = CurrentInput.IsDrifting() && GetCurrentSpeedAbsolute() >= Settings.Steering.DriftMinSpeed;
}

void UArcadeVehicleMovementComponentBase::GetWheelsBaseTransform(FTransform& OutTransform) const
{
	OutTransform = PhysicsPrimitive->GetComponentTransform();
}

void UArcadeVehicleMovementComponentBase::CalculateGravity(float DeltaSeconds)
{
	/* Only apply custom gravity when enabled. */
	if(!Settings.Physics.EnableCustomGravity)
	{
		return;
	}
	
	/* Calculate gravity force. */
	const FVector gravityForce = PhysicsPrimitive->GetMass() * CustomGravity;

	/* Apply gravity force. */
	PhysicsPrimitive->AddForce(gravityForce);
}

void UArcadeVehicleMovementComponentBase::CalculateSuspension(float DeltaSeconds)
{
	/*  Prepare line trace settings. We do not want to trace the vehicle itself. */
	FCollisionQueryParams traceParams;
	traceParams.AddIgnoredActor(GetOwner());
	FCollisionObjectQueryParams queryParams = FCollisionObjectQueryParams::DefaultObjectQueryParam;
	for(ECollisionChannel collisionChannel : Settings.Suspension.CollisionChannels)
	{
		queryParams.AddObjectTypesToQuery(collisionChannel);
	}

	/* Calculate trace direction. */
	const FVector suspensionTraceDirection = -PhysicsPrimitive->GetUpVector() * Settings.Suspension.TraceLength;

	/* Reset total wheel count. */
	WheelsInfo = FVehicleWheelsRuntimeInfo();

	/* Iterate over all suspension springs. */
	for (FVehicleSuspensionSpring& spring : Settings.Suspension.Springs)
	{
		/* Count steering and drive wheels. */
		if(spring.bIsSteeringWheel)
		{
			WheelsInfo.SteeringWheelsCount++;
		}
		if(spring.bIsDriveWheel)
		{
			WheelsInfo.DriveWheelsCount++;
		}

		/* Cache suspension parent bone transform, or if not valid, use component transform. */
		FTransform componentTransform = PhysicsPrimitive->GetComponentTransform();
		FTransform wheelTransform;
		GetWheelsBaseTransform(wheelTransform);
	
		/* Convert suspension location from local to world space. */
		FVector suspensionWorld = componentTransform.TransformPositionNoScale(spring.Location);
		FVector wheelWorld = wheelTransform.TransformPositionNoScale(spring.Location);

		/* Calculate up offset to compensate for thin surfaces. */
		const FVector suspensionOffset = PhysicsPrimitive->GetUpVector() * Settings.Suspension.TraceUpOffset;

		/* Establish hit validity. */
		FHitResult hitResultSuspension;
		
		/* Handle ray or sphere cast. */
		const bool bUseLineTrace = Settings.Suspension.TraceThickness <= 0.f;
		if(!bUseLineTrace)
		{
			FCollisionShape traceShape = FCollisionShape::MakeSphere(Settings.Suspension.TraceThickness);
			spring.LatestTrace.IsHitValid = GetWorld()->SweepSingleByObjectType(hitResultSuspension, suspensionWorld + suspensionOffset, suspensionWorld + suspensionTraceDirection, FQuat::Identity, queryParams, traceShape, traceParams);
		}
		else
		{
			spring.LatestTrace.IsHitValid = GetWorld()->LineTraceSingleByObjectType(hitResultSuspension, suspensionWorld + suspensionOffset, suspensionWorld + suspensionTraceDirection, queryParams, traceParams);
		}

		/* If we have a valid hit and we are using trace up offset. */
		if(spring.LatestTrace.IsHitValid && Settings.Suspension.TraceUpOffset > 0.f)
		{
			/* Compensate for the trace up offset. */
			hitResultSuspension.Distance -= Settings.Suspension.TraceUpOffset;
			hitResultSuspension.TraceStart -= suspensionOffset;

			/* When using sphere cast we still want to keep the center of the sphere as if it was ray. The distance given by the sphere cast isn't the same, so we need to correct that. */
			if(!bUseLineTrace)
			{
				hitResultSuspension.Distance = (hitResultSuspension.ImpactPoint - hitResultSuspension.TraceStart).Size();
			}

			/* If the distance if below 0, prevent ground sinking. */
			if(hitResultSuspension.Distance < 0.f)
			{
				PhysicsPrimitive->SetWorldLocation(PhysicsPrimitive->GetComponentLocation() + PhysicsPrimitive->GetUpVector() * -hitResultSuspension.Distance);
				componentTransform = PhysicsPrimitive->GetComponentTransform();
				GetWheelsBaseTransform(wheelTransform);

				/* Distance should not go below 0. */
				hitResultSuspension.Distance = 0.f;
			}
		}
		
		/* Store the hit data for the latest trace. */
		spring.LatestTrace.BeginLocation = suspensionWorld;
		spring.LatestTrace.EndLocation = hitResultSuspension.ImpactPoint;
		spring.LatestTrace.Normal = hitResultSuspension.ImpactNormal;
		spring.LatestTrace.Distance = hitResultSuspension.Distance;
		spring.WheelOffset = suspensionWorld.Z - wheelWorld.Z;

		/* Count up wheels on the ground. */
		if (spring.LatestTrace.IsHitValid)
		{
			/* Calculate suspension force at this location. */
			const FVector suspensionPointVelocity = PhysicsPrimitive->GetPhysicsLinearVelocityAtPoint(hitResultSuspension.TraceStart);

			/* Calculate how different the velocity of the bone is to the normal that the trace found. */
			const float relativeVelocity = FVector::DotProduct(suspensionPointVelocity, hitResultSuspension.Normal);
			
			/* Calculate final suspension force multiplier. */
			const float springFinalForce = -GetVehicleGravity().Size() * -spring.SpringForce;
			const float dampingForce = spring.Damping * (Settings.Suspension.EnableSuspensionStabilization ? Settings.Suspension.SuspensionStabilizationMultiplier * DeltaSeconds : 1.f);
			const float springFinalDamping = dampingForce * relativeVelocity;
			float forceMultiplier = -(springFinalForce * (hitResultSuspension.Distance - spring.TargetHeight)) - springFinalDamping;

			/* Springs should never push vehicle downwards, which is what negative spring force would do. */
			forceMultiplier = Settings.Suspension.EnableGroundSnapping ? forceMultiplier : FMath::Max(0.f, forceMultiplier);
			
			/* Calculate final force vector. */
			spring.LatestSpringForce = PhysicsPrimitive->GetUpVector() * forceMultiplier;

			/* Bump up wheel contact. */
			if(spring.bIsSteeringWheel)
			{
				WheelsInfo.SteeringWheelsOnGround++;
			}
			if(spring.bIsDriveWheel)
			{
				WheelsInfo.DriveWheelsOnGround++;
			}
		}
	}

	/* Iterate over all springs once again. */
	for (FVehicleSuspensionSpring& spring : Settings.Suspension.Springs)
	{
		/* Cache latest trace as it's frequently used. */
		const FVehicleSuspensionSpringTrace& latestTrace = spring.LatestTrace;

		/* Apply spring force if hit. */
		if (spring.LatestTrace.IsHitValid)
		{
			/* Apply final spring force. */
			PhysicsPrimitive->AddForceAtLocation(spring.LatestSpringForce, latestTrace.BeginLocation);

			/* Find the new center offset the wheel from the raycast hit location. We will then go back by the wheel radius. */
			/* Wheel radius needs to be entered manually. */
			float wheelCenterOffset = -(latestTrace.Distance - spring.WheelRadius);

			/* Then what we want to do is to take the wheel center offset calculated from the raycast, and clamp it using provided limits. */
			wheelCenterOffset = FMath::Clamp(wheelCenterOffset + spring.WheelOffset, spring.MinMaxOffsetZ.X, spring.MinMaxOffsetZ.Y);

			/* Apply final offset. */
			spring.WheelOffset = wheelCenterOffset;

			/* Calculate and apply swing amount. */
			spring.CurrentSwing = FMath::GetMappedRangeValueClamped(spring.MinMaxOffsetZ, spring.SwingMinMax, spring.WheelOffset);
		}
		/* If the trace has not hit anything. */
		else 
		{
			/* Nothing was hit, we will set wheel offset of this spring to be absolute max. */
			spring.WheelOffset = spring.MinMaxOffsetZ.X;

			/* Calculate and apply swing amount. */
			spring.CurrentSwing = FMath::GetMappedRangeValueClamped(spring.MinMaxOffsetZ, spring.SwingMinMax, spring.WheelOffset);
		}
	}
}

void UArcadeVehicleMovementComponentBase::CalculateAdherence(float DeltaTime, float& OutLinearAdherence, float& OutAngularAdherence)
{
	/* Handle drifting by modulating mutable linear damping. */
	OutLinearAdherence = Settings.Steering.LinearDamping;
	OutAngularAdherence = Settings.Steering.AngularDamping;
	if (PhysicsRuntime.bIsDrifting)
	{
		/* When drifting adherence goes down immediately. */
		PhysicsRuntime.AdherenceMultiplier = Settings.Steering.DriftAdherencePercentage;
		PhysicsRuntime.RotationMultiplier = Settings.Steering.DriftRotationPercentage;
	}
	/* If not drifting. */
	else
	{
		/* Adherence will increase linearly. Max is always 1. */
		PhysicsRuntime.AdherenceMultiplier = 
		FMath::Clamp
		(
		PhysicsRuntime.AdherenceMultiplier + Settings.Steering.DriftRecoverySpeed * DeltaTime,
		Settings.Steering.DriftAdherencePercentage, 1.f
		);

		/* Do the same to the rotation. */
		PhysicsRuntime.RotationMultiplier =
		FMath::Clamp
		(
		PhysicsRuntime.RotationMultiplier + Settings.Steering.DriftRecoverySpeed * DeltaTime,
		Settings.Steering.DriftRotationPercentage, 1.f
		);
	}

	/* Apply final adherence multiplier. */
	OutLinearAdherence *= PhysicsRuntime.AdherenceMultiplier;
	OutAngularAdherence *= PhysicsRuntime.AdherenceMultiplier;
}

FVector UArcadeVehicleMovementComponentBase::CalculateAcceleration(float DeltaTime, float LinearAdherence)
{
	/* Turn current linear velocity direction into rotation, and blend the rotation with the forward orientation to retain pitch and roll. */
	const FVector realLocalLinearVelocity = PhysicsRuntime.LocalLinearVelocity;

	/* Grab current orientation of the velocity vector, but use forward vector Yaw to keep side or up drag of the vehicle, but modulate acceleration vector. */
	FRotator realLocalRotation = realLocalLinearVelocity.Rotation();
	realLocalRotation.Yaw = FVector::ForwardVector.Rotation().Yaw;

	/* Calculate acceleration natural vector, which is the orientation of the current velocity, but we enhance X axis, which is vehicle forward. */
	FVector accelNaturalVector = realLocalRotation.Vector();

	/* Accelerate backward or forward. */
	accelNaturalVector.X *= FMath::Sign(GetCurrentSpeed());
	
	/* Calculate initial linear velocity for this frame using damping value. */
	const FVector targetLocalLinearVelocity = accelNaturalVector * realLocalLinearVelocity.Size();

	/* Damp real linear velocity using current forward direction of the vehicle and blend between them. */
	FVector finalLinearVelocity = FMath::Lerp(realLocalLinearVelocity, targetLocalLinearVelocity, FMath::Clamp(LinearAdherence * DeltaTime, 0.f, 1.f));

	/* Check if the vehicle is moving at all. */
	const bool bIsMoving = !FMath::IsNearlyZero(GetCurrentSpeed(), 0.2f);

	/* Check if the vehicle has acceleration applied in any direction. */
	const bool bHasAcceleration = LastForces.Acceleration != 0.f;
	const bool bCanAccelerate = bHasAcceleration && GetCurrentSpeedUnit() < 1.f;

	/* When so slow as 0.2, we will apply zero linear velocity to make sure vehicle stops. Obviously this is only something we can do when there is no acceleration from the user. */
	if (!bIsMoving && !bHasAcceleration)
	{
		/* Only zero X axis of the linear velocity. */
		finalLinearVelocity = FVector(0.f, finalLinearVelocity.Y, finalLinearVelocity.Z);
	}

	/* Check if the vehicle acceleration is opposite to its movement direction. Will be false if there is no acceleration applied at all, or if vehicle isn't moving at all. */
	const bool bHasOppositeAcceleration = bIsMoving && bHasAcceleration && FMath::Sign(LastForces.Acceleration) != FMath::Sign(GetCurrentSpeed());

	/* Apply engine braking flag. This happens, when the vehicle is moving, and it has no acceleration input applied. */
	PhysicsRuntime.bIsEngineBraking = bIsMoving && !bHasAcceleration;

	/* Apply general braking flag, which is uber flag of the engine braking. This one can be triggered by either engine braking, or the opposite direction acceleration. */
	PhysicsRuntime.bIsBraking = PhysicsRuntime.bIsEngineBraking || bHasOppositeAcceleration;

	/* Reset last applied forces. */
	LastForces.LastAppliedBraking = 0.f;
	LastForces.LastAppliedAcceleration = 0.f;

	/* Apply engine braking. */
	if (PhysicsRuntime.bIsEngineBraking)
	{
		/* Cache linear velocity before calculating braking, so we can calculate the difference and see how much we braked. */
		const float cachedVelocity = finalLinearVelocity.Size();

		/* Calculate engine braking force from its curve. */
		const float brakingForce = LastForces.EngineBraking * DeltaTime;
		const float brakingAlpha = FMath::Clamp(brakingForce, 0.f, 1.f);

		/* Calculate the velocity we are blending into for engine braking. Engine braking should not affect any other direction than X. */
		const FVector brakingTargetVelocity(0.f, finalLinearVelocity.Y, finalLinearVelocity.Z);
		finalLinearVelocity = FMath::Lerp(finalLinearVelocity, brakingTargetVelocity, brakingAlpha);

		/* Calculate last applied braking force. */
		LastForces.LastAppliedBraking = cachedVelocity - finalLinearVelocity.Size();
	}
	/* If not engine braking, check if braking at all. */
	else if (PhysicsRuntime.bIsBraking)
	{
		/* This is braking due to the acceleration being opposite to the current vehicle movement. */
		const float brakingForce = LastForces.Braking * DeltaTime;
		finalLinearVelocity -= finalLinearVelocity.GetSafeNormal() * brakingForce;

		/* Calculate last applied braking force. */
		LastForces.LastAppliedBraking = brakingForce;
	}
	/* Only apply any acceleration if accelerating. */
	else if (bCanAccelerate)
	{
		/* Find drive multiplier if needed. */
		const float driveMultiplier = (Settings.Engine.bScaleAccelerationByDriveWheels ? WheelsInfo.GetDriveWheelsMultiplier() : 1.f);
		
		/* Cache last applied acceleration. */
		LastForces.LastAppliedAcceleration = LastForces.Acceleration * driveMultiplier * DeltaTime;

		/* Simply apply move forward direction to the velocity. */
		const FVector velocityDelta = realLocalRotation.Vector() * LastForces.LastAppliedAcceleration;
		finalLinearVelocity += velocityDelta;

		/* Mark as accelerating. */
		PhysicsRuntime.bIsAccelerating = true;
	}
	
	/* Return final velocity. */
	finalLinearVelocity.Z = realLocalLinearVelocity.Z;
	return finalLinearVelocity;
}

void UArcadeVehicleMovementComponentBase::CalculateFriction(float DeltaTime, FVector& LinearVelocity)
{	
	/* No friction correction is applied while drifting. This is based on the adherence multiplier that is driven by the drift and clamped always at 1. */
	if (PhysicsRuntime.AdherenceMultiplier < 1.f)
	{
		return;
	}

	/* Friction is always multiplied by the wheels being on ground. */
	const float wheelsFrictionFactor = WheelsInfo.GetTotalWheelsMultiplier();

	/* Calculate friction force alpha. */
	const float frictionForceAlpha = 1.f - FMath::Clamp(FMath::Abs(LinearVelocity.Y * KMH_MULTIPLIER) / Settings.Physics.FrictionForceThreshold, 0.f, 1.f);

	/* Find opposite or zero force by using friction force. */
	LinearVelocity.Y = FMath::Lerp(LinearVelocity.Y, -LinearVelocity.Y, FMath::Clamp(Settings.Physics.FrictionForce * frictionForceAlpha * wheelsFrictionFactor, 0.f, 1.f));

	/* Check if total friction should be applied this frame. */	
	const float latestSped = FMath::Abs(LinearVelocity.X * KMH_MULTIPLIER);
	const bool bApplyTotalFriction = Settings.Physics.TotalFrictionSpeedThreshold > 0.f && frictionForceAlpha > 0.f && latestSped <= Settings.Physics.TotalFrictionSpeedThreshold;

	/* Check should apply total friction this frame. */
	if (bApplyTotalFriction)
	{
		/* If it previously didn't have it on, cache it here. */
		if (!PhysicsRuntime.bHasLastTotalFriction)
		{
			PhysicsRuntime.TotalFrictionSnapLocation = PhysicsPrimitive->GetComponentLocation();
		}
		/* If it did have it last frame as well, reapply it. */
		else
		{
			/* Calculate alpha for this frame of the total friction. */
			const float totalFrictionAlpha = FMath::Clamp(latestSped / Settings.Physics.TotalFrictionSpeedThreshold, 0.f, 1.f);

			/* Calculate location offset. */
			FVector locationOffset = PhysicsRuntime.TotalFrictionSnapLocation - PhysicsPrimitive->GetComponentLocation();

			/* Unrotate offset vector so we have local space, we do not want to move anything else than the local Y. */
			const FRotator componentRotation = PhysicsPrimitive->GetComponentRotation();
			locationOffset = componentRotation.UnrotateVector(locationOffset);
			locationOffset.X = 0.f;
			locationOffset.Z = 0.f;

			/* Lerp final offset using friction alpha. */
			locationOffset.Y = FMath::Lerp(locationOffset.Y, 0.f, totalFrictionAlpha);

			/* Apply vehicle local offset. */
			PhysicsPrimitive->AddLocalOffset(locationOffset, true, nullptr, ETeleportType::TeleportPhysics);

			/* Calculate this frame component location to update it for the friction snap. */
			PhysicsRuntime.TotalFrictionSnapLocation = PhysicsPrimitive->GetComponentLocation();
		}
	}

	/* Cache new state of the total friction. */
	PhysicsRuntime.bHasLastTotalFriction = bApplyTotalFriction;
}

FVector UArcadeVehicleMovementComponentBase::CalcuateAngularAdherence(float AlphaTime, float AngularAdherence, const FVector& InAngularVelocity)
{
	/* Grab the initial angular velocity for this frame with damping value. */
	const FVector targetAngularVelocity = FVector::ZeroVector;

	/* Calculate initial with damping. */
	const float alpha = FMath::Clamp(AngularAdherence * AlphaTime, 0.f, 1.f);
	return FMath::Lerp(InAngularVelocity, targetAngularVelocity, alpha);
}

void UArcadeVehicleMovementComponentBase::CalculateTurning(float DeltaTime, FVector& InOutAngularVelocity)
{
	/* Skip if not turning. */
	if(LastForces.Turning == 0.f)
	{
		return;
	}

	/*
	 * If steering is not allowed during braking, and we are braking right now, we won't allow turning.
	 * However, engine braking doesn't count.
	 */
	if(!Settings.Steering.AllowSteeringWhileBraking && PhysicsRuntime.bIsBraking && !PhysicsRuntime.bIsEngineBraking)
	{
		return;
	}

	/* Calculate this frame turning delta. Smooth rotation using resting speed. */
	float frameDeltaRotation = LastForces.Turning * PhysicsRuntime.RotationMultiplier * DeltaTime;

	/* Rotate final angular velocity for this frame's rotation. */
	InOutAngularVelocity.Z += frameDeltaRotation;
}

void UArcadeVehicleMovementComponentBase::OnReceiveState_Server_Implementation(const FVehiclePhysicsState& State)
{
	/* Set this most up to date server state in order to replicate it to everyone. */
	ServerState = State;

	/* Calculate server state timestamp. We need to grab current server time, and remove half RTT from it, so we know when client has completed this state. */
	ServerState.TimeStamp = GetHalfRTT();
	
	/* If server receiving here is also in control of this vehicle, he doesn't want to buffer anything. */
	if(!HasControlOverVehicle())
	{
		/* Call replicated method on the server to buffer the state. */
		OnRep_ServerState();
	}
}

void UArcadeVehicleMovementComponentBase::OnReceiveTeleport_Server_Implementation(const FVector_NetQuantize& Location, const FRotator& Rotation)
{
	OnReceiveTeleport_Client(Location, Rotation);
}

void UArcadeVehicleMovementComponentBase::OnReceiveTeleport_Client_Implementation(const FVector_NetQuantize& Location, const FRotator& Rotation)
{
	/* Cache last teleport time. */
	LastTeleportTime = GetWorld()->GetTimeSeconds();
	
	/* Cleanup all the network data so far. */
	ClearNetworkData();

	/* Actually teleport this vehicle. Reset physics, absolutely. */
	PhysicsPrimitive->SetWorldLocationAndRotation(Location, Rotation, false, nullptr, ETeleportType::TeleportPhysics);
	PhysicsPrimitive->SetPhysicsLinearVelocity(FVector::ZeroVector);
	PhysicsPrimitive->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);

	/* Ensure we snap vehicle using total friction at this point. */
	PhysicsRuntime.TotalFrictionSnapLocation = Location;
	PhysicsRuntime.bHasLastTotalFriction = true;
}

void UArcadeVehicleMovementComponentBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UArcadeVehicleMovementComponentBase, ServerState, COND_SkipOwner);
	DOREPLIFETIME(UArcadeVehicleMovementComponentBase, MaxSpeedMultiplier);
}

FVehiclePhysicsState UArcadeVehicleMovementComponentBase::BuildState() const
{
	FVehiclePhysicsState outputState;
	outputState.Location = PhysicsPrimitive->GetComponentLocation();
	outputState.Rotation = PhysicsPrimitive->GetComponentRotation();
	outputState.LinearVelocity = PhysicsPrimitive->GetPhysicsLinearVelocity();
	outputState.AngularVelocity = PhysicsPrimitive->GetPhysicsAngularVelocityInDegrees();
	outputState.Input = CurrentInput;
	outputState.SetMovementModifiers(PhysicsRuntime.MovementModifiers);
	outputState.TimeStamp = GetWorld()->GetTimeSeconds();
	return outputState;
}

void UArcadeVehicleMovementComponentBase::OnRep_ServerState()
{
	/* We don't care about states received before we have began play. */
	if (!HasBegunPlay())
	{
		return;
	}

	/* Calculate when this state was originally dispatched. Arriving value contains half RTT of the owner, we will add our half RTT to it. */
	const float localHalfRTT = GetHalfRTT();
	const float localTimeStamp = GetWorld()->GetTimeSeconds() - (ServerState.TimeStamp + localHalfRTT);

	/* If time stamp is older than last teleport known, do not accept it. */
	if(localTimeStamp < LastTeleportTime)
	{
		return;
	}

	/* Grab the most suitable state we have completed at this time. */
	FVehiclePhysicsState stateFromPast;
	if(StateBuffer.GetSuitableState(localTimeStamp, stateFromPast))
	{
		/* Setup corrections if needed. */
		LocationCorrection.ValueOfCorrection = ServerState.Location;
		LocationCorrection.ErrorValue = ServerState.Location - stateFromPast.Location;
		LocationCorrection.bIsCorrecting = LocationCorrection.ErrorValue.Size() > 1.f;
		//
		RotationCorrection.ValueOfCorrection = ServerState.Rotation.Quaternion();
		RotationCorrection.ErrorValue = stateFromPast.Rotation.GetInverse().Quaternion() * ServerState.Rotation.Quaternion();
		RotationCorrection.bIsCorrecting = AngularDistance(RotationCorrection.ErrorValue, FQuat::Identity) > 1.f;
		//
		LinearVelocityCorrection.ValueOfCorrection = ServerState.LinearVelocity;
		LinearVelocityCorrection.ErrorValue = ServerState.LinearVelocity - stateFromPast.LinearVelocity;
		LinearVelocityCorrection.bIsCorrecting = LinearVelocityCorrection.ErrorValue.Size() > 1.f;
		//
		AngularVelocityCorrection.ValueOfCorrection = ServerState.AngularVelocity;
		AngularVelocityCorrection.ErrorValue = ServerState.AngularVelocity - stateFromPast.AngularVelocity;
		AngularVelocityCorrection.bIsCorrecting = AngularVelocityCorrection.ErrorValue.Size() > 1.f;

		/* Remove old states from buffer, they won't be of any use. */
		StateBuffer.ClearOldStates(localTimeStamp + localHalfRTT);
	}
}

float UArcadeVehicleMovementComponentBase::GetHalfRTT() const
{
	if(IsValid(GetPawnOwner()) && IsValid(GetPawnOwner()->GetPlayerState()))
	{
		return GetPawnOwner()->GetPlayerState()->ExactPing * 0.001f * 0.5f;
	}

	return 0.f;
}

void UArcadeVehicleMovementComponentBase::ApplyPhysicsCorrections()
{
	/* Cache some values for shorter usage. */
	const float exponent = Settings.Physics.PhysicsCorrectionExponential;
	
	/* For each type of correction flow is the same. If correcting, lerp error towards zero and offset vehicle slightly. */
	if(LocationCorrection.bIsCorrecting)
	{
		/* Check if we want to correct or snap. */
		if(LocationCorrection.ErrorValue.Size() > Settings.Physics.PhysicsLocationSnapDistance)
		{
			PhysicsPrimitive->SetWorldLocation(LocationCorrection.ValueOfCorrection, false, nullptr, ETeleportType::TeleportPhysics);
			LocationCorrection.bIsCorrecting = false;
		}
		else
		{
			if(Settings.Physics.bEnhancePhysicsCorrection)
			{
				LocationCorrection.ErrorValue = LocationCorrection.ValueOfCorrection - PhysicsPrimitive->GetComponentLocation();
			}
			LocationCorrection.ErrorValue = FMath::Lerp(LocationCorrection.ErrorValue, FVector::ZeroVector, Settings.Physics.PhysicsCorrectionExponential);
			PhysicsPrimitive->AddWorldOffset(LocationCorrection.ErrorValue, false, nullptr, ETeleportType::TeleportPhysics);
			LocationCorrection.bIsCorrecting = LocationCorrection.ErrorValue.Size() > 1.f;
		}

		/* If we have total friction. */
		if(PhysicsRuntime.bHasLastTotalFriction)
		{
			/* Update total friction snap location. This is to prevent local friction from blocking net corrections. */
			PhysicsRuntime.TotalFrictionSnapLocation = PhysicsPrimitive->GetComponentLocation();
		}
	}
	if(RotationCorrection.bIsCorrecting)
	{
		/* Check if we want to correct or snap. */
		if(AngularDistance(RotationCorrection.ErrorValue, FQuat::Identity) > Settings.Physics.PhysicsRotationSnapDistance)
		{
			RotationCorrection.bIsCorrecting = false;
			PhysicsPrimitive->SetWorldRotation(RotationCorrection.ValueOfCorrection, false, nullptr, ETeleportType::TeleportPhysics);
		}
		else
		{
			if(Settings.Physics.bEnhancePhysicsCorrection)
			{
				RotationCorrection.ErrorValue = PhysicsPrimitive->GetComponentRotation().GetInverse().Quaternion() * RotationCorrection.ValueOfCorrection;
			}
			RotationCorrection.ErrorValue = FQuat::Slerp(RotationCorrection.ErrorValue, FQuat::Identity, Settings.Physics.PhysicsCorrectionExponential);
			PhysicsPrimitive->AddWorldRotation(RotationCorrection.ErrorValue, false, nullptr, ETeleportType::TeleportPhysics);
			RotationCorrection.bIsCorrecting = AngularDistance(RotationCorrection.ErrorValue, FQuat::Identity) > 1.f;
		}
	}
	if(LinearVelocityCorrection.bIsCorrecting)
	{
		if(Settings.Physics.bEnhancePhysicsCorrection)
		{
			LinearVelocityCorrection.ErrorValue = LinearVelocityCorrection.ValueOfCorrection - PhysicsPrimitive->GetPhysicsLinearVelocity();
		}
		LinearVelocityCorrection.ErrorValue = FMath::Lerp(LinearVelocityCorrection.ErrorValue, FVector::ZeroVector, exponent);
		LinearVelocityCorrection.bIsCorrecting = LinearVelocityCorrection.ErrorValue.Size() > 1.f;
		PhysicsPrimitive->SetPhysicsLinearVelocity(PhysicsPrimitive->GetPhysicsLinearVelocity() + LinearVelocityCorrection.ErrorValue);
	}
	if(AngularVelocityCorrection.bIsCorrecting)
	{
		if(Settings.Physics.bEnhancePhysicsCorrection)
		{
			AngularVelocityCorrection.ErrorValue = AngularVelocityCorrection.ValueOfCorrection - PhysicsPrimitive->GetPhysicsAngularVelocityInDegrees();
		}
		AngularVelocityCorrection.ErrorValue = FMath::Lerp(AngularVelocityCorrection.ErrorValue, FVector::ZeroVector, exponent);
		AngularVelocityCorrection.bIsCorrecting = AngularVelocityCorrection.ErrorValue.Size() > 1.f;
		PhysicsPrimitive->SetPhysicsAngularVelocityInDegrees(PhysicsPrimitive->GetPhysicsAngularVelocityInDegrees() + AngularVelocityCorrection.ErrorValue);
	}
}

float UArcadeVehicleMovementComponentBase::AngularDistance(const FQuat& A, const FQuat& B)
{
	return FMath::RadiansToDegrees(A.AngularDistance(B));
}
