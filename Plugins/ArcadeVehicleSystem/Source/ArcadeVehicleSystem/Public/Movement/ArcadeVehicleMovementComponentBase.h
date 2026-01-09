/** Created and owned by Furious Production LTD @ 2023. **/

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Networking/ArcadeVehicleNetworkHelpers.h"
#include "Settings/ArcadeVehicleSettings.h"
#include "ArcadeVehicleMovementComponentBase.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogArcadeVehicleMovement, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCalculateCustomVehicleMovement, UPrimitiveComponent*, InVehiclePhysicsMesh, const FVehicleInputState&, Input, float, DeltaSeconds);

class UArcadeVehiclePathFollowingComponent;

/** Accessible constant for converting UE velocity units to km/h. */
static const float KMH_MULTIPLIER = 0.036f;

/**
	Component that is responsible for calculating and synchronizing vehicle
	movement physics. This is arcade-like vehicle physics.
	It is base component and should not be used, but derived from.
*/
UCLASS(Abstract, BlueprintType)
class ARCADEVEHICLESYSTEM_API UArcadeVehicleMovementComponentBase : public UPawnMovementComponent
{
	GENERATED_BODY()

public:
	UArcadeVehicleMovementComponentBase();

	/** UActorComponent interface. */
	void BeginPlay() override;
	void RegisterComponentTickFunctions(bool bRegister) override;
	void SetComponentTickEnabled(bool bEnabled) override;
	void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	/** ~UActorComponent interface. */

	/** UPawnMovementComponent interface. */
	void RequestPathMove(const FVector& MoveInput) override;
	bool IsMovingOnGround() const override;
	bool IsFalling() const override;
	/** ~UPawnMovementComponent interface. */

	/** Just an accessor method for the settings. They are constant - can't change. */
	const FVehicleSettings& GetVehicleSettings() const;
	
	/**
	 * Sets full set of settings for this vehicle. Use wisely!
	 * Calling this will uninitialize vehicle physics meaning that the vehicle will not move and will not be calculated for safety.
	 * If calling this at runtime, it is required to call ApplyVehicleSettings after that, so the vehicle is re-initialized.
	 * If you are setting this before BeginPlay is called, for example in constructor,
	 * ApplyVehicleSettings shouldn't be called. It will be called on movement component BeginPlay anyway.
	 */
	UFUNCTION(BlueprintCallable, Category=VehicleSettings)
	void SetVehicleSettings(const FVehicleSettings& NewSettings);

	/**
	 * Sets full set of Physics for this vehicle. Use wisely!
	 * Calling this will uninitialize vehicle physics meaning that the vehicle will not move and will not be calculated for safety.
	 * If calling this at runtime, it is required to call ApplyVehicleSettings after that, so the vehicle is re-initialized.
	 * If you are setting this before BeginPlay is called, for example in constructor,
	 * ApplyVehicleSettings shouldn't be called. It will be called on movement component BeginPlay anyway.
	 */
	UFUNCTION(BlueprintCallable, Category=VehicleSettings)
	void SetPhysicsSettings(const FVehiclePhysicsSettings& NewSettings);
	
	/**
	 * Sets full set of Engine for this vehicle. Use wisely!
	 * Calling this will uninitialize vehicle physics meaning that the vehicle will not move and will not be calculated for safety.
	 * If calling this at runtime, it is required to call ApplyVehicleSettings after that, so the vehicle is re-initialized.
	 * If you are setting this before BeginPlay is called, for example in constructor,
	 * ApplyVehicleSettings shouldn't be called. It will be called on movement component BeginPlay anyway.
	 */
	UFUNCTION(BlueprintCallable, Category=VehicleSettings)
	void SetEngineSettings(const FVehicleEngineSettings& NewSettings);

	/**
	 * Sets full set of Steering for this vehicle. Use wisely!
	 * Calling this will uninitialize vehicle physics meaning that the vehicle will not move and will not be calculated for safety.
	 * If calling this at runtime, it is required to call ApplyVehicleSettings after that, so the vehicle is re-initialized.
	 * If you are setting this before BeginPlay is called, for example in constructor,
	 * ApplyVehicleSettings shouldn't be called. It will be called on movement component BeginPlay anyway.
	 */
	UFUNCTION(BlueprintCallable, Category=VehicleSettings)
	void SetSteeringSettings(const FVehicleSteeringSettings& NewSettings);

	/**
	 * Sets full set of Suspension for this vehicle. Use wisely!
	 * Calling this will uninitialize vehicle physics meaning that the vehicle will not move and will not be calculated for safety.
	 * If calling this at runtime, it is required to call ApplyVehicleSettings after that, so the vehicle is re-initialized.
	 * If you are setting this before BeginPlay is called, for example in constructor,
	 * ApplyVehicleSettings shouldn't be called. It will be called on movement component BeginPlay anyway.
	 */
	UFUNCTION(BlueprintCallable, Category=VehicleSettings)
	void SetSuspensionSettings(const FVehicleSuspensionSettings& NewSettings);
	
	/**
	 * If vehicle settings were changed at runtime, this needs to be called to apply them. It also reinitializes vehicle physics entirely.
	 * It won't affect movement or networking, or current velocity of the vehicle.
	 */
	UFUNCTION(BlueprintCallable, Category=VehicleSettings)
	void ApplyVehicleSettings();

	/** Returns current gravity that works for this vehicle. This can be the default or custom gravity if enabled. */
	UFUNCTION(BlueprintPure, Category=Gravity)
	FVector GetVehicleGravity() const;
	
	/** Sets custom gravity for this vehicle. This only works when physics settings have EnableCustomGravity checked!. */
	UFUNCTION(BlueprintCallable, Category=Gravity)
	void SetCustomGravity(const FVector& InGravity);
	
	/** Returns number of drive wheels currently on ground. */
	UFUNCTION(BlueprintPure, Category=Wheels)
	int32 GetNumberOfDriveWheelsOnGround() const;

	/** Returns number of steering wheels currently on ground. */
	UFUNCTION(BlueprintPure, Category=Wheels)
	int32 GetNumberOfSteeringWheelsOnGround() const;

	/** Returns total number of wheels currently on ground. */
	UFUNCTION(BlueprintPure, Category=Wheels)
	int32 GetNumberOfWheelsOnGround() const;
	
	/** Returns raw turning input for this vehicle. */
	UFUNCTION(BlueprintPure, Category = Movement)
	float GetTurningInput() const;

	/** Returns current speed of the vehicle, but it will be negative when the vehicle is reversing and positive if moving forward. */
	UFUNCTION(BlueprintPure, Category = Movement)
	float GetCurrentSpeed() const;

	/** Returns current speed of the vehicle as absolute value, so it doesn't matter whether the vehicle is moving forward of backward, it will just return speed. */
	UFUNCTION(BlueprintPure, Category = Movement)
	float GetCurrentSpeedAbsolute() const;

	/** Returns current speed of the vehicle defined as 0-1 unit from 0 to max speed. */
	UFUNCTION(BlueprintPure, Category = Movement)
	float GetCurrentSpeedUnit() const;

	/** Returns current local linear velocity of this vehicle. */
	UFUNCTION(BlueprintPure, Category = Movement)
	FVector GetLocalLinearVelocity() const;

	/** Returns world-space linear velocity, but only forward without right-left or up-down direction. */
	UFUNCTION(BlueprintPure, Category = Movement)
	FVector GetLinearVelocityForward() const;

	/** Returns world-space linear velocity, but only right without forward-backward or up-down direction. */
	UFUNCTION(BlueprintPure, Category = Movement)
	FVector GetLinearVelocityRight() const;
	
	/** Returns last applied known acceleration in any direction. */
	UFUNCTION(BlueprintPure, Category = Movement)
	float GetLastAppliedAcceleration() const;

	/** Returns last applied know braking in any direction. */
	UFUNCTION(BlueprintPure, Category = Movement)
	float GetLastAppliedBraking() const;

	/** Sets max speed of this vehicle immediately. */
	UFUNCTION(BlueprintCallable, Category = Movement)
	void SetMaxSpeedMultiplier(const float NewMaxSpeedMultiplier);
	
	/** Returns current max speed multiplier. */
	UFUNCTION(BlueprintPure, Category = Movement)
	float GetMaxSpeedMultiplier() const;

	/** Sets current acceleration input. It is automatically clamped between -1 and 1. */
	UFUNCTION(BlueprintCallable, Category = Input)
	void SetAccelerationInput(const float Value);

	/** Sets current turning input. It is automatically clamped between -1 and 1. */
	UFUNCTION(BlueprintCallable, Category = Input)
	void SetTurningInput(const float Value);

	/** Sets current drift input. Value of false will stop drifting, and value of true will start it. */
	UFUNCTION(BlueprintCallable, Category = Input)
	void SetDriftInput(const bool EnableDrift);

	/** Sets current stabilization input. Value of false will stop stabilizing, and value of true will start it. */
	UFUNCTION(BlueprintCallable, Category = Input)
	void SetStabilizationInput(const bool EnableStabilization);

	/** Sets custom float input. */
	UFUNCTION(BlueprintCallable, Category = Input)
	void SetCustomInput(float Value);

	/** Sets custom bitflag input. */
	UFUNCTION(BlueprintCallable, Category = Input)
	void SetCustomBitflagInput(uint8 Bitflag, bool bEnableFlag);

	/** Checks custom bitflag input. */
	UFUNCTION(BlueprintPure, Category = Input)
	static bool CheckCustomBitflagInput(int32 Bitflag, const FVehicleInputState& Input);
	
	/** Checks if vehicle is currently accelerating, so moving forward. */
	UFUNCTION(BlueprintPure, Category = Movement)
	bool IsAccelerating() const;

	/** Checks if the vehicle is currently moving forwards. It doesn't mean accelerating, but just direction of the movement. */
	UFUNCTION(BlueprintPure, Category = Movement)
	bool IsMovingForward() const;
		
	/** Checks if the vehicle is currently moving backwards. It doesn't mean accelerating, but just direction of the movement. */
	UFUNCTION(BlueprintPure, Category = Movement)
	bool IsMovingBackward() const;

	/** Check if the vehicle is currently moving at all, that'd be forward or backward. */
	UFUNCTION(BlueprintPure, Category = Movement)
	bool IsMovingAtAll() const;

	/** Returns whether or not this vehicle is currently braking(not the same as reversing). */
	UFUNCTION(BlueprintPure, Category = Movement)
	bool IsBraking() const;

	/** Returns specifically state of the engine braking. */
	UFUNCTION(BlueprintPure, Category = Movement)
	bool IsEngineBraking() const;

	/** Returns current state of drifting for this vehicle. */
	UFUNCTION(BlueprintPure, Category = Movement)
	bool IsDrifting() const;

	/** Teleports this vehicle to the provided transform and clears the physics state. */
	UFUNCTION(BlueprintCallable, Category = Physics)
	void TeleportVehicle(const FTransform& Transform);

	/** Sets current linear velocity to the physics of the vehicle. Should only be called on the host-side. Velocity can be set in local or world space. */
	UFUNCTION(BlueprintCallable, Category = Physics)
	void SetVehicleLinearVelocity(const FVector& LinearVelocity, const bool WorldSpace = false);
	
	/** Returns wheel offset for the wheel of specified index. */
	UFUNCTION(BlueprintCallable, Category = Suspension)
	float GetWheelOffset(int32 Index) const;

	/** Returns acceleration blocking flag. */
	UFUNCTION(BlueprintCallable, Category = Acceleration)
	bool IsAccelerationBlocked() const;
	
	/** Function that allows to blocking acceleration. */
	UFUNCTION(BlueprintCallable, Category = Acceleration)
	void SetBlockAcceleration(bool Block);

	/** Returns vehicle simulation mesh, which is the root component. */
	UFUNCTION(BlueprintPure, Category = VehicleMesh)
	UPrimitiveComponent* GetVehicleMesh() const;

	/** Templated method for returning vehicle root mesh. Use wisely! */
	template<typename T>
	T* GetVehicleMeshT() const { return Cast<T>(PhysicsPrimitive); }
	
	/** Clears network data of this vehicle. */
	UFUNCTION(BlueprintCallable, Category = Networking)
	void ClearNetworkData();

	/** This clears all inputs in this vehicle. It is called automatically when needed, but if you have some custom events, override them and clear them here. */
	virtual void ClearInputs();

	/** Returns path following component associated. */
	const UArcadeVehiclePathFollowingComponent* GetPathFollowingComponent();

	/** Checks if we have control over this vehicle. */
	bool HasControlOverVehicle() const;
	
protected:
	/** Ticks before physics. */
	virtual void OnPrePhysicsTick(float DeltaTime);

	/** Ticks after physics. */
	virtual void OnPostPhysicsTick(float DeltaTime);

	/** Prepares simulation frame. It will cache state we are going to be starting from. */
	virtual void PrepareFrame();
	
	/** Gathers inputs before the simulation. */
	virtual void GatherInputs();

	/** Calculates forces for incoming frame. */
	virtual void CalculateForces();

	/**
	 * Called during BeginPlay phase of this component. It should initialize vehicle root component
	 * reference, and register everything that's needed for the vehicle to work.
	 * Should return whether or not we have successfully registered physics data completely.
	 */
	virtual bool InitializeVehicleMovement();
	
	/**
	 * Logic for registering suspension springs.
	 * Depending on the implementation it might mean
	 * skeletal mesh bones or some custom way of handling them.
	 * Should return true when implemented and successful.
	 * It will affect initialization outcome.
	 */
	virtual bool RegisterSuspensionSprings();

	/** Calculates physics runtime information for the simulation to have it in one place. */
	virtual void CalculatePhysicsRuntimeData();

	/**
	 * Returns wheels transform. It is transform that allows to offset wheels
	 * for specific parent bone if needed.
	 * Will return root component transform if not overriden.
	 */
	virtual void GetWheelsBaseTransform(FTransform& OutTransform) const;

	/** Calculates gravity for this vehicle. */
	virtual void CalculateGravity(float DeltaSeconds);
	
	/** Calculates suspension using raycasts. */
	virtual void CalculateSuspension(float DeltaSeconds);

	/** Calculates adherence forces. */
	virtual void CalculateAdherence(float DeltaTime, float& OutLinearAdherence, float& OutAngularAdherence);

	/** Calculates and outputs acceleration and braking as linear velocity. */
	virtual FVector CalculateAcceleration(float DeltaTime, float LinearAdherence);

	/** Calculates friction force. Takes linear velocity after acceleration calculations and chews it with the friction forces. */
	virtual void CalculateFriction(float DeltaTime, FVector& LinearVelocity);

	/** Calculates angular adherence. */
	virtual FVector CalcuateAngularAdherence(float AlphaTime, float AngularAdherence, const FVector& InAngularVelocity);
	
	/** Calculates and outputs turning related maths as angular velocity. */
	virtual void CalculateTurning(float DeltaTime, FVector& InOutAngularVelocity);
	
	/** Rpc called on the server when the client owning the vehicle sends its state. */
	UFUNCTION(Server, Unreliable)
	void OnReceiveState_Server(const FVehiclePhysicsState& State);
	virtual void OnReceiveState_Server_Implementation(const FVehiclePhysicsState& State);

	/** Rpc called on the server when the client owning the vehicle sends teleport command. */
	UFUNCTION(Server, Reliable)
	void OnReceiveTeleport_Server(const FVector_NetQuantize& Location, const FRotator& Rotation);
	virtual void OnReceiveTeleport_Server_Implementation(const FVector_NetQuantize& Location, const FRotator& Rotation);

	/** Rpc called on all of the clients when the server receives teleport command from the owning client. */
	UFUNCTION(NetMulticast, Reliable)
	void OnReceiveTeleport_Client(const FVector_NetQuantize& Location, const FRotator& Rotation);
	virtual void OnReceiveTeleport_Client_Implementation(const FVector_NetQuantize& Location, const FRotator& Rotation);
	
	/** Register members for syncing. */
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Returns state structure based on current physical properties. */
	FVehiclePhysicsState BuildState() const;

	/** Called when server state arrives to client. */
	UFUNCTION()
	void OnRep_ServerState();

	/** Returns half round-trip-time of the client owning this vehicle. */
	float GetHalfRTT() const;

	/** Applies given physics corrections. */
	virtual void ApplyPhysicsCorrections();

	/** Calculates angular distance between two rotations in degrees. */
	static float AngularDistance(const FQuat& A, const FQuat& B);

public:
	/**
	* Allows to implement additional custom movement logic. Called after all regular calculations.
	* Apply forces to the vehicle physical mesh in this function to apply your custom movement.
	*/
	UPROPERTY(BlueprintAssignable, Category=CustomMovement)
	FCalculateCustomVehicleMovement CalculateCustomVehicleMovement;
	
protected:
	/** Input state assigned for local player during pressing buttons etc. */
	UPROPERTY(BlueprintReadOnly, Category=Input)
	FVehicleInputState LocalInput;

	/** Defines max speed multiplier. It allows to quickly lock vehicle speed, and unlock it at will. */
	UPROPERTY(Replicated)
	float MaxSpeedMultiplier;

	/** All of the vehicle settings split into groups. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleSettings)
	FVehicleSettings Settings;

protected:
	/**
	 * Assigned mesh of this vehicle. This should always be the root component possibly.
	 * Also this is the component responsible for simulation of this vehicle.
	 */
	UPROPERTY()
	UPrimitiveComponent* PhysicsPrimitive;

	/**
	 * Whether or not this vehicle is fully initialized.
	 * This is assigned during the initialization phase and
	 * is true if the references are valid.
	 * It won't allow any ticking, at any point in time if invalid.
	 */
	bool bIsVehicleInitialized;

	/**
	 * Stores runtime wheels information.
	 * Simply to avoid putting here another few variables.
	 */
	FVehicleWheelsRuntimeInfo WheelsInfo;

	/** Cached path following component. */
	UPROPERTY()
	UArcadeVehiclePathFollowingComponent* PathFollowingComponent;
	
private:
	/** Error correction. */
	VectorCorrectionData LocationCorrection;
	QuatCorrectionData RotationCorrection;
	VectorCorrectionData LinearVelocityCorrection;
	VectorCorrectionData AngularVelocityCorrection;

	/** Current vehicle gravity force and direction. */
	FVector CustomGravity;
	
	/** State replicated from server to all clients. */
	UPROPERTY(ReplicatedUsing=OnRep_ServerState)
	FVehiclePhysicsState ServerState;

	FVehiclePhysicsStateArray StateBuffer;
	
	/** Tick that happens before physics. */
	FActorComponentTickFunction PrePhysicsTick;

	/** Tick that happens after physics. */
	FActorComponentTickFunction PostPhysicsTick;

	/** Stores previously applied forces of the vehicle. Updated at each post physics tick. */
	FVehicleForces LastForces;

	/** Stores previously calculated runtime physical properties. */
	FVehiclePhysicsRuntime PhysicsRuntime;

	/** Input currently used for the simulation. Assigned at the beginning of each frame. */
	FVehicleInputState CurrentInput;

	/** Defines local time of last teleportation event. It ensures no physics states are applied, that are older than this information. */
	float LastTeleportTime;

	/** Controller current having control over this vehicle. Cached to diff changes. */
	UPROPERTY()
	AController* CurrentController;
};