/** Created and owned by Furious Production LTD @ 2023. **/

#pragma once
#include "Engine/EngineTypes.h"
#include "Containers/EnumAsByte.h"
#include "ArcadeVehicleSettings.generated.h"

class UCurveFloat;

/**
	Grouped settings of the physics of the vehicle.
*/
USTRUCT(BlueprintType)
struct ARCADEVEHICLESYSTEM_API FVehiclePhysicsSettings
{
	GENERATED_BODY()

	FVehiclePhysicsSettings();

	/** When enabled, vehicle will use custom gravity provided by the user. */
	UPROPERTY(EditAnywhere, Category = Physics)
	bool EnableCustomGravity;
	
	/** Defines how strong is the stabilization force. */
	UPROPERTY(EditAnywhere, Category = Physics)
	float StabilizationForce;

	/** Defines friction force of this vehicle. It zeroes out Y velocity movment and lerps with current using the force. 0 will do nothing, 1 will use force opposite to the current. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Engine, meta=(UIMin="0.0", ClampMin="0.0", UIMax="1.0", ClampMax="1.0"))
	float FrictionForce;

	/** Defines friction force threshold. If the vehicle is pushed stronger than this value, the side friction won't be applied. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Engine, meta = (UIMin = "0.0", ClampMin = "0.0"))
	float FrictionForceThreshold;

	/** Defines what speed needs to be reached in order to achieve full friction - locking vehicle in place. 0 will never lock it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Engine, meta = (UIMin = "0.0", ClampMin = "0.0"))
	float TotalFrictionSpeedThreshold;

	/** Defines tolerance which allows vehicle to decide whether it is moving forward or backward. Useful for many scenarios to ease out error with low speeds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Engine, meta = (UIMin = "0.0", ClampMin = "0.0"))
	float MovementDirectionTolerance;
	
	/** Defines exponential parameter for the error correction. */
	UPROPERTY(EditAnywhere, Category = PhysicsCorrection)
	float PhysicsCorrectionExponential;

	/** Defines what is the location distance beyond which system will snap vehicle location instead of correcting. */
	UPROPERTY(EditAnywhere, Category = PhysicsCorrection)
	float PhysicsLocationSnapDistance;

	/** Defines what is the rotation distance beyond which system will snap vehicle rotation instead of correcting. */
	UPROPERTY(EditAnywhere, Category = PhysicsCorrection)
	float PhysicsRotationSnapDistance;

	/** Enables enhanced networked physics correction. This feature aims to fix physics correction issues when the ping is high. */
	UPROPERTY(EditAnywhere, Category = PhysicsCorrection)
	bool bEnhancePhysicsCorrection;
};

/**
	Grouped settings of the vehicle engine.
*/
USTRUCT(BlueprintType)
struct ARCADEVEHICLESYSTEM_API FVehicleEngineSettings
{
	GENERATED_BODY()

	FVehicleEngineSettings();

	/** Special curve used to scale acceleration forward force. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Engine)
	UCurveFloat* AccelerationCurve;

	/** Special curve used to scale acceleration backwards force. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Engine)
	UCurveFloat* ReversingCurve;

	/** Special curve used to scale engine braking force, which is applied when there is no acceleration input. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Engine)
	UCurveFloat* EngineBrakingCurve;

	/** Special curve used to scale regular braking force, which is applied when the acceleration direction is opposite to the direction of heading of the vehicle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Engine)
	UCurveFloat* BrakingCurve;

	/** Defines maximum speed of this vehicle. No acceleration will be applied when this value is reached. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Engine)
	float MaxSpeed;

	/** Defines maximum reversing speed. No reverse acceleration will be applied when this value is reached. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Engine)
	float MaxReverseSpeed;

	/**
	 * If this is true, each drive wheel will contribute to the acceleration
	 * so if we have two drive wheels, and only one touches the ground,
	 * the acceleration force will be 50%.
	 * If this is false, then the acceleration will not be scaled this way, and the
	 * acceleration will be always the same as long as there
	 * is at least one drive wheel touching the ground.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Engine)
	bool bScaleAccelerationByDriveWheels;
};

/**
	Grouped settings of the vehicle steering.
*/
USTRUCT(BlueprintType)
struct ARCADEVEHICLESYSTEM_API FVehicleSteeringSettings
{
	GENERATED_BODY()

	FVehicleSteeringSettings();

	/** Special curve used to scale turning force of the vehicle based on its absolute speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Steering)
	UCurveFloat* SteeringCurve;

	/** When enabled, vehicle will be allowed to turn while braking. By default vehicle cannot turn during braking. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Steering)
	bool AllowSteeringWhileBraking;
	
	/** Defines amount of turn damping. This damping is used to avoid hard-snap feeling when releasing turn input. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Steering)
	float SteeringDamping;

	/** Defines strength of the linear damping. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Steering)
	float LinearDamping;

	/** Defines strength of the angular damping. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Steering)
	float AngularDamping;

	/** Defines how many percent of regular adherence there is when drifting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Steering)
	float DriftAdherencePercentage;

	/** Defines how many percent of regular rotation there is when drifting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Steering)
	float DriftRotationPercentage;

	/** Defines how fast vehicle will recover its full adherence after drifting. It's speed multiplier, not a time. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Steering)
	float DriftRecoverySpeed;

	/** Defines minimal speed required to drift. It won't activate below this speed. It is defined by KM/h just like vehicle speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Steering)
	float DriftMinSpeed;
};

/**
	Helper structure that allows to store less info for the FHitResult
	for the springs, as we do not need all of the data.
*/
USTRUCT()
struct ARCADEVEHICLESYSTEM_API FVehicleSuspensionSpringTrace
{
	GENERATED_BODY()

	FVehicleSuspensionSpringTrace();

	/** Location where the hit was shot from. */
	UPROPERTY()
	FVector BeginLocation;

	/** Location of the hit. */
	UPROPERTY()
	FVector EndLocation;

	/** Normal of the hit. */
	UPROPERTY()
	FVector Normal;

	/** Distance of the hit for the suspension force. */
	UPROPERTY()
	float Distance;

	/** Whether or not the hit is valid. */
	UPROPERTY()
	bool IsHitValid;
};

/**
	Helper structure that allows to map suspension
	spring physics data with the name of the bone,
	where the spring belongs.
*/
USTRUCT(BlueprintType)
struct ARCADEVEHICLESYSTEM_API FVehicleSuspensionSpring
{
	GENERATED_BODY()

	FVehicleSuspensionSpring();

	/** Defines the name of the bone that is used to locate the spring in runtime. If this is using static mesh, enter socket name instead. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Suspension)
	FName BoneName;
	
	/** Defines how strong is this spring. Higher values will apply stronger force upwards. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Suspension)
	float SpringForce;

	/** Defines height of the suspension at this particular spring. The system will try to settle this spring at this height. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Suspension)
	float TargetHeight;

	/** Damping factor allows to define how suspension damps the impacts. Kind of like softness. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Suspension)
	float Damping;

	/** Defines whether this spring contains a wheel that is steering wheel. Value used purely for animations, not for physics. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Suspension)
	bool bIsSteeringWheel;

	/**
	 * Defines whether this spring contains a wheel that is drive wheel. Actually affects acceleration.
	 * Drive wheels that have connection to the ground will allow applying acceleration.
	 * This means, that if there are 2 drive wheels, and only one touches the ground,
	 * only 50% force will be applied.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Suspension)
	bool bIsDriveWheel;
	
	/** Defines radius of the wheel we expect to attach to this suspension part. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Suspension)
	float WheelRadius;
	
	/** Defines where the swing pivot should be from 0 to 1 along the SwingMinMax value. Smaller values will begin earlier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Suspension, meta=(UIMin="0.0", UIMax="1.0", ClampMin="0.0", ClampMax="1.0"))
	float SwingPivot;

	/** Defines behaviour of the swing arm. Min and max value corresponds with the value in the wheels offsets min and max. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Suspension)
	FVector2D SwingMinMax;

	/** Defines minimum and maximum offset for this spring wheel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Suspension)
	FVector2D MinMaxOffsetZ;

	/** Non-exposed value. It is just location of the bone associated with this spring in component-space. */
	UPROPERTY()
	FVector Location;

	/** Stores runtime-calculated wheel offset on this spring. */
	UPROPERTY()
	float WheelOffset;

	/** Runtime value that defines current swing arm swing amount. */
	UPROPERTY()
	float CurrentSwing;

	/** Runtime value. Stores information about the last performed line trace for this string. */
	UPROPERTY()
	FVehicleSuspensionSpringTrace LatestTrace;

	/** Runtime value. Stores lates spring force calculated. */
	UPROPERTY()
	FVector LatestSpringForce;
};

/**
	Grouped settings of the vehicle suspension.
*/
USTRUCT(BlueprintType)
struct ARCADEVEHICLESYSTEM_API FVehicleSuspensionSettings
{
	GENERATED_BODY()

	FVehicleSuspensionSettings();

	/** Defines collision channels that the suspension will target. In other words - what the vehicle can drive upon. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Suspension)
	TArray<TEnumAsByte<ECollisionChannel>> CollisionChannels;
	
	/** Defines how far suspension line traces reach. This allows to define how the vehicle is snapped to the ground. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Suspension)
	float TraceLength;

	/** Defines additional trace offset towards the top of the vehicle to avoid suspension sinking into the ground. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension|Advanced", meta=(UIMin="0.0", ClampMin="0.0"))
	float TraceUpOffset;
	
	/** Defines thickness of the suspension trace. When the thickness is 0, it will just use raycast, otherwise sphere cast. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Suspension)
	float TraceThickness;

	/** 
		Name of the bone that the suspension bones should be transformed to world by.
		This one is important, because if we are doing some kind of animations on that bone,
		and it is a parent of the spring bones, we will want to keep them up-to-date
		with their parent transform. This could be a root bone, or bone after that most likely.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Suspension)
	FName SuspensionParentBoneName;

	/**
	 * When true, suspension will push vehicle towards the ground when it lifts off the ground,
	 * as long as the raycasts touch the ground.
	 * Normally suspension only pushes upwards.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Suspension)
	bool EnableGroundSnapping;

	/**
	 * When true, suspension stabilization feature is enabled.
	 * This feature ensures, that suspension doesn't become unstable
	 * with low-fps scenarios.
	 * Note, that damping might feel different when this feature is enabled,
	 * so it should be re-tweaked accordingly.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Suspension)
	bool EnableSuspensionStabilization;
	
	/**
	 * Additional damping value for the suspension. Overall damping is now multiplied by the delta time
	 * to account for suspension stability in lower FPS scenarios.
	 * Therefore overall damping force decreased significantly.
	 * This allows to multiply damping force so it's experimentally closer to our previous desire.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Suspension, meta=(EditCondition="EnableSuspensionStabilization==true", EditConditionHides))
	float SuspensionStabilizationMultiplier;

	/** List of the springs that drive suspension of this vehicle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Suspension)
	TArray<FVehicleSuspensionSpring> Springs;

	/** Runtime value that stores index of the suspension parent bone for quicker access. */
	UPROPERTY()
	int32 SuspensionParentBoneIndex;
};

/**
	Grouped settings of the vehicle advanced stuff.
	Only access and change these when you absolutely know what you're doing.
*/
USTRUCT(BlueprintType)
struct ARCADEVEHICLESYSTEM_API FVehicleAdvancedSettings
{
	GENERATED_BODY()

	FVehicleAdvancedSettings();

	/** Disables vehicle simulation completely when it's running in sequencer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Advanced)
	bool bDisablePhysicsInSequencer;
	
	/** Allows to completely disable suspension calculations. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Advanced)
	bool bEnableSuspension;

	/** Allows to completely disable adherence calculations. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Advanced)
	bool bEnableAdherence;

	/** Allows to completely disable acceleration calculations. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Advanced)
	bool bEnableAcceleration;

	/** Allows to completely disable turning calculations. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Advanced)
	bool bEnableTurning;

	/** Allows to completely disable friction calculations. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Advanced)
	bool bEnableFriction;
};

/** Groups all of vehicle settings. They are in one place so they are very easy to copy and pase if needed. */
USTRUCT(BlueprintType)
struct ARCADEVEHICLESYSTEM_API FVehicleSettings
{
	GENERATED_BODY()

	FVehicleSettings();

	/** Physics settings of this vehicle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Physics)
	FVehiclePhysicsSettings Physics;

	/** Engine settings of this vehicle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Engine)
	FVehicleEngineSettings Engine;

	/** Steering settings of this vehicle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Steering)
	FVehicleSteeringSettings Steering;

	/** Steering settings of this vehicle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Suspension)
	FVehicleSuspensionSettings Suspension;

	/** Advanced settings of the vehicle. Only if you know what you are doing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Advanced)
	FVehicleAdvancedSettings Advanced;
};

/**
 * Runtime information about the vehicle wheels.
 * It's only here to gather this information in one place
 * not to make too big mess in the actual component.
 */
USTRUCT()
struct ARCADEVEHICLESYSTEM_API FVehicleWheelsRuntimeInfo
{
	GENERATED_BODY()

	FVehicleWheelsRuntimeInfo();

	/** Returns smooth percentage of how many drive wheels are on the ground versus how many there are in total. */
	float GetDriveWheelsMultiplier() const;
	float GetTotalWheelsMultiplier() const;
	
	/** How many steering wheels there are are in total. */
	UPROPERTY()
	int32 SteeringWheelsCount;

	/** How many drive wheels there are in total. */
	UPROPERTY()
	int32 DriveWheelsCount;

	/** How many steering wheels are currently on the ground. */
	UPROPERTY()
	int32 SteeringWheelsOnGround;

	/** How many drive wheels are currently on the ground. */
	UPROPERTY()
	int32 DriveWheelsOnGround;
};