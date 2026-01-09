/** Created and owned by Furious Production LTD @ 2023. **/

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SpringArmComponent.h"
#include "ArcadeVehicleCameraController.generated.h"

class UCineCameraComponent;
class APlayerController;

/**
	Enumerator that defines current state of the camera snapping sequence.
*/
UENUM(BlueprintType)
enum class CameraSnappingSequenceState : uint8
{
	None,
	Interpolating,
	Snapped,
	Blocked,
};

/**
	Enumerator that specifies pitch state of the owner vehicle.
*/
UENUM(BlueprintType)
enum class CameraOwnerPitchState : uint8
{
	Down,
	Flat,
	Up
};

/**
	Helper structure used to gather values used for camera snapping.
	Easier to keep them in one bag.
*/
USTRUCT(BlueprintType)
struct ARCADEVEHICLESYSTEM_API FCameraSnappingData
{
	GENERATED_BODY()

	FCameraSnappingData();

	/** Rotation cached on begin play as snap rotation for the flat surfaces. */
	UPROPERTY(BlueprintReadOnly, Category=CameraSnapping)
	FRotator SnapRotation;

	/** Defines current state of the camera snapping. */
	UPROPERTY(BlueprintReadOnly, Category=CameraSnapping)
	CameraSnappingSequenceState State;

	/** Timer used for blocked state to calculate how long it should be blocked for. */
	UPROPERTY(BlueprintReadOnly, Category=CameraSnapping)
	float BlockTimer;
};

/**
	Extension for the spring arm component that implements custom rotation sampling.
*/
UCLASS(BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class ARCADEVEHICLESYSTEM_API UArcadeVehicleCameraController : public USpringArmComponent
{
	GENERATED_BODY()

public:
	UArcadeVehicleCameraController();

	/** UActorComponent interface. */
	void BeginPlay() override;
	void Activate(bool bReset = false) override;
	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	/** UActorComponent interface. */
	
	/** Sets references to the cameras managed by this controller. */
	UFUNCTION(BlueprintCallable, Category = Initialization)
	void InitializeCameras(UArcadeVehicleMovementComponentBase* MovementComponent, UCineCameraComponent* FrontCamera, UCineCameraComponent* RearCamera, USpringArmComponent* RearCameraArm);
	
	/** Activates front camera and deactivates rear one. */
	UFUNCTION(BlueprintCallable, Category = Activation)
	void ActivateFrontCamera();

	/** Activates rear camera and deactivates front one. */
	UFUNCTION(BlueprintCallable, Category = Activation)
	void ActivateRearCamera();

	/** Deactivates all cameras. */
	UFUNCTION(BlueprintCallable, Category = Activation)
	void DeactivateAllCameras();

	/** Checks if the front camera is active. */
	UFUNCTION(BlueprintPure, Category = Activation)
	bool IsFrontCameraActive();

	/** Applies camera yaw input. */
	UFUNCTION(BlueprintCallable, Category = Input)
	void AddYawInput(float Value);

	/** Applies camera pitch input. */
	UFUNCTION(BlueprintCallable, Category = Input)
	void AddPitchInput(float Value);
	
	/** Begins camera snapping sequence. Note that if snapping is disabled, this will silently bail. */
	UFUNCTION(BlueprintCallable, Category = Snapping)
	void BeginSnappingSequence();

	/** Stops latest camera snapping sequence completely. */
	UFUNCTION(BlueprintCallable, Category = Snapping)
	void StopSnappingSequence();

	/** Stops currently ongoing snapping sequence if any, and blocks potentially new one for given amount of time. */
	UFUNCTION(BlueprintCallable, Category = Snapping)
	void BlockSnappingSequence();

	/** Clears currently snapped state. */
	UFUNCTION(BlueprintCallable, Category = Snapping)
	void ClearSnappedState();

	/** Sets owner speed to this component. */
	UFUNCTION(BlueprintCallable, Category = SpeedEffects)
	void UpdateOwnerSpeed(float Speed);
	
	/** Updates owner turn angle for this controller. */
	UFUNCTION(BlueprintCallable, Category = SpeedEffects)
	void UpdateTurnAngle(float TurnAngle);

	/** USpringArmComponent interface. */
	FRotator GetDesiredRotation() const override;
	/** ~USpringArmComponent interface. */

	/** Just a utility method that allows to fix rotator so that it uses -180 to 180 degrees. */
	static void RotatorPitchTo180(FRotator& OutRotator);

protected:
	/** Internal method that evaluates initialization flag validity. */
	void EvaluateInitializationFlag();
	
	/** Calculates yaw axis snapping. */
	void CalculateYawSnapping(float DeltaTime);

	/** Calculates current pitch snap rotation based on the owner pitch state. */
	FRotator CalculateTargetPitchRotation();

	/** Calculates pitch axis snapping. */
	void CalculatePitchSnapping(float DeltaTime);

	/** Calculates camera speed effects. */
	void CalculateCameraSpeedEffects(float DeltaTime);

	/** Checks if camera owner is locally controlled. */
	bool IsLocallyControlled() const;

public:
	/** Defines time used for this camera to be blended in. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = General)
	float CameraPossessingBlendTime;

	/** Value that enables and disables capability of the rotation snapping at all. Even starting snap sequence won't work when this is false. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Snapping)
	bool bUseRotationSnapping;

	/** Value that enables and disables pitch helper at all. Only takes effect if rotation snapping is enabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Snapping)
	bool bUsePitchHelper;

	/** Delay used to begin scheduled snapping sequence. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Snapping)
	float RotationSnappingDelay;

	/** Defines speed of the camera snapping rotation for the Yaw axis. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Snapping)
	float YawSnappingSpeed;

	/** Defines speed of the camera snapping rotation for the Pitch axis. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Snapping)
	float PitchSnappingSpeed;

	/** Defines tolerance needed for the camera to be considered snapped. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Snapping)
	float RotationSnappingTolerance;

	/** Defines how many degrees of the pitch is required for the owner to trigger pitch blending. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PitchHelper)
	float PitchThreshold;

	/** Defines how much pitch rotation is added to the pitch on slopes up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PitchHelper)
	float PitchUpwardAngularOffset;

	/** Defines how much pitch rotation is added to the pitch on slopes down. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PitchHelper)
	float PitchDownwardAngularOffset;

	/** Defines range of the speeds used for the focal length calculations. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SpeedEffects)
	FVector2D FocalLengthSpeedRange;

	/** Defines range of the focal length values that will be mapped to the speeds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SpeedEffects)
	FVector2D FocalLengthValuesRange;

	/** Defines range of the speeds used for the camera approach calculations. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SpeedEffects)
	FVector2D CameraApproachSpeedRange;

	/** Defines range of the camera approach values that will be mapped to the speeds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SpeedEffects)
	FVector2D CameraApproachValuesRange;

private:
	/** Flag set upon controller initialization, when all the refs are valid to use. */
	UPROPERTY()
	bool m_bIsControllerInitialized;
		
	/** Movement component of the vehicle currently using this camera controller. */
	UPROPERTY()
	UArcadeVehicleMovementComponentBase* m_pMovementComponent;	
		
	/** Camera pointing forward vehicle. */
	UPROPERTY()
	UCineCameraComponent* m_pFrontCamera;

	/** Camera pointing backward vehicle. */
	UPROPERTY()
	UCineCameraComponent* m_pRearCamera;

	/** Camera spring arm parent used for the rear camera. */
	UPROPERTY()
	USpringArmComponent* m_pRearCameraArm;

	/** Local player controller. Value is cached for quicker access. */
	UPROPERTY()
	APlayerController* m_pLocalPlayerController;

	/** Keeps all information about the yaw axis snapping sequence. */
	UPROPERTY()
	FCameraSnappingData m_yawSnappingData;

	/** Keeps all information about the pitch axis snapping sequence. */
	UPROPERTY()
	FCameraSnappingData m_pitchSnappingData;

	/** Tracks last known owner pitch state. */
	UPROPERTY()
	CameraOwnerPitchState m_ownerPitchState;

	/** Tracks owner turning angle. */
	UPROPERTY()
	float m_ownerTurnAngle;

	/** Cached owner's speed as alpha to be used for calculations. */
	UPROPERTY()
	float m_ownerSpeed;

	/** Defines whether or not owner's speed has changed to avoid unnecessary calculations. */
	UPROPERTY()
	bool m_bIsOwnerSpeedDirty;
};