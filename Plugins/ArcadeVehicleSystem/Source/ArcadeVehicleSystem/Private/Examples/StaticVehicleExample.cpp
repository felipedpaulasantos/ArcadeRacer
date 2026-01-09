// Fill out your copyright notice in the Description page of Project Settings.

#include "Examples/StaticVehicleExample.h"
#include "Examples/StaticVehicleAnimatorExample.h"
#include "Movement/StaticArcadeVehicleMovementComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Curves/CurveFloat.h"

AStaticVehicleExample::AStaticVehicleExample()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	InitializeVehicleProperties();
}

void AStaticVehicleExample::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("Accelerate", MovementComponent, &UStaticArcadeVehicleMovementComponent::SetAccelerationInput);
	PlayerInputComponent->BindAxis("Turn", MovementComponent, &UStaticArcadeVehicleMovementComponent::SetTurningInput);
}

void AStaticVehicleExample::InitializeVehicleProperties()
{
	/* Setup vehicle meshes. */
	static ConstructorHelpers::FObjectFinder<UStaticMesh> vehicleMesh(TEXT("StaticMesh'/ArcadeVehicleSystem/Meshes/SportCar/SportCar_StaticMesh.SportCar_StaticMesh'"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> wheelMesh(TEXT("StaticMesh'/ArcadeVehicleSystem/Meshes/SportCar/SportCarWheel_StaticMesh.SportCarWheel_StaticMesh'"));

	/* Setup vehicle curves. */
	static ConstructorHelpers::FObjectFinder<UCurveFloat> accelerationCurve(TEXT("CurveFloat'/ArcadeVehicleSystem/Curves/AccelerationCurve'"));
	static ConstructorHelpers::FObjectFinder<UCurveFloat> reversingCurve(TEXT("CurveFloat'/ArcadeVehicleSystem/Curves/ReversingCurve'"));
	static ConstructorHelpers::FObjectFinder<UCurveFloat> engineBrakingCurve(TEXT("CurveFloat'/ArcadeVehicleSystem/Curves/EngineBrakingCurve'"));
	static ConstructorHelpers::FObjectFinder<UCurveFloat> brakingCurve(TEXT("CurveFloat'/ArcadeVehicleSystem/Curves/BrakingCurve'"));
	static ConstructorHelpers::FObjectFinder<UCurveFloat> steeringCurve(TEXT("CurveFloat'/ArcadeVehicleSystem/Curves/SteeringCurve'"));

	/* Set vehicle mesh asset to the appropriate component. */
	VehicleMesh->SetStaticMesh(vehicleMesh.Object); // This mesh is used for actual physics calculations.
	VehicleMesh->SetCollisionProfileName("Vehicle");
	VehicleMesh->SetSimulatePhysics(true);
	VehicleMesh->SetGenerateOverlapEvents(false);
	VehicleMesh->SetVisibility(false, false);
	//
	VehicleVisualMesh->SetStaticMesh(vehicleMesh.Object); // This mesh is for visual purposes only. They are both required.
	VehicleVisualMesh->SetCollisionProfileName("NoCollision");
	VehicleVisualMesh->SetSimulatePhysics(false);
	VehicleVisualMesh->SetGenerateOverlapEvents(false);

	/* This will store new vehicle settings. */
	FVehicleSettings newSettings;

	/* Setup engine settings. */
	newSettings.Engine.AccelerationCurve = accelerationCurve.Object;
	newSettings.Engine.ReversingCurve = reversingCurve.Object;
	newSettings.Engine.EngineBrakingCurve = reversingCurve.Object;
	newSettings.Engine.BrakingCurve = brakingCurve.Object;
	newSettings.Engine.MaxSpeed = 180.f;
	newSettings.Engine.MaxReverseSpeed = 50.f;
	newSettings.Engine.bScaleAccelerationByDriveWheels = true;

	/* Setup physics settings. */
	newSettings.Physics.StabilizationForce = 30.f;
	newSettings.Physics.FrictionForce = 0.7f;
	newSettings.Physics.FrictionForceThreshold = 5.f;
	newSettings.Physics.TotalFrictionSpeedThreshold = 5.f;
	newSettings.Physics.MovementDirectionTolerance = 0.f;
	newSettings.Physics.PhysicsCorrectionExponential = 0.9f;
	
	/* Setup steering settings. */
	newSettings.Steering.SteeringCurve = steeringCurve.Object;
	newSettings.Steering.SteeringDamping = 0.5f;
	newSettings.Steering.LinearDamping = 1.f;
	newSettings.Steering.AngularDamping = 1.f;
	newSettings.Steering.DriftAdherencePercentage = 0.1f;
	newSettings.Steering.DriftRotationPercentage = 0.1f;
	newSettings.Steering.DriftRecoverySpeed = 0.1f;
	newSettings.Steering.DriftMinSpeed = 30.f;

	/* Setup suspension settings. */
	newSettings.Suspension.TraceLength = 80.f;
	newSettings.Suspension.TraceThickness = 15.f;
	newSettings.Suspension.SuspensionParentBoneName = "Suspension";

	/* Suspension settings contains springs setup, which is array of springs, usually 4 for regular vehicles. They are required for physics to even work. */
	FVehicleSuspensionSpring frontLeftSpring;
	frontLeftSpring.BoneName = "Wheel_Front_Left";
	frontLeftSpring.SpringForce = 30.f;
	frontLeftSpring.TargetHeight = 45.f;
	frontLeftSpring.Damping = 1800.f;
	frontLeftSpring.bIsSteeringWheel = true;
	frontLeftSpring.bIsDriveWheel = true;
	frontLeftSpring.WheelRadius = 35.f;
	frontLeftSpring.SwingPivot = 0.5f;
	frontLeftSpring.SwingMinMax = FVector2D(-2.5f, 7.f);
	frontLeftSpring.MinMaxOffsetZ = FVector2D(-20.f, 5.f);
	//
	FVehicleSuspensionSpring frontRightSpring = frontLeftSpring; // Copy left front spring, it's almost the same.
	frontRightSpring.BoneName = "Wheel_Front_Right";
	frontRightSpring.SwingMinMax *= -1.f; // Swinging on the other side of the vehicle must be inverted.
	//
	FVehicleSuspensionSpring rearLeftSpring = frontLeftSpring; // Copy left front spring, it's almost the same. 
	rearLeftSpring.BoneName = "Wheel_Rear_Left";
	rearLeftSpring.bIsSteeringWheel = false;
	//
	FVehicleSuspensionSpring rearRightSpring = rearLeftSpring; // Copy left front spring, it's almost the same.
	rearRightSpring.BoneName = "Wheel_Rear_Right";
	rearRightSpring.SwingMinMax *= -1.f; // Swinging on the other side of the vehicle must be inverted.
	//
	newSettings.Suspension.Springs.Add(frontLeftSpring);
	newSettings.Suspension.Springs.Add(frontRightSpring);
	newSettings.Suspension.Springs.Add(rearLeftSpring);
	newSettings.Suspension.Springs.Add(rearRightSpring);

	/* This is complete settings setup now, we can set it to the movement component. */
	MovementComponent->SetVehicleSettings(newSettings);
	
	/* Setup wheel meshes. */
	UStaticMeshComponent* pFrontLeftWheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrontLeftWheel"));
	pFrontLeftWheel->SetStaticMesh(wheelMesh.Object);
	pFrontLeftWheel->SetCollisionProfileName("NoCollision");
	pFrontLeftWheel->SetSimulatePhysics(false);
	pFrontLeftWheel->SetGenerateOverlapEvents(false);
	pFrontLeftWheel->SetupAttachment(VehicleVisualMesh, frontLeftSpring.BoneName);
	//
	UStaticMeshComponent* pFrontRightWheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrontRightWheel"));
	pFrontRightWheel->SetStaticMesh(wheelMesh.Object);
	pFrontRightWheel->SetCollisionProfileName("NoCollision");
	pFrontRightWheel->SetSimulatePhysics(false);
	pFrontRightWheel->SetGenerateOverlapEvents(false);
	pFrontRightWheel->SetupAttachment(VehicleVisualMesh, frontRightSpring.BoneName);
	pFrontRightWheel->SetRelativeRotation(FRotator(0.f, -180.f, 0.f));
	//
	UStaticMeshComponent* pRearLeftWheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RearLeftWheel"));
	pRearLeftWheel->SetStaticMesh(wheelMesh.Object);
	pRearLeftWheel->SetCollisionProfileName("NoCollision");
	pRearLeftWheel->SetSimulatePhysics(false);
	pRearLeftWheel->SetGenerateOverlapEvents(false);
	pRearLeftWheel->SetupAttachment(VehicleVisualMesh, rearLeftSpring.BoneName);
	//
	UStaticMeshComponent* pRearRightWheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RearRightWheel"));
	pRearRightWheel->SetStaticMesh(wheelMesh.Object);
	pRearRightWheel->SetCollisionProfileName("NoCollision");
	pRearRightWheel->SetSimulatePhysics(false);
	pRearRightWheel->SetGenerateOverlapEvents(false);
	pRearRightWheel->SetupAttachment(VehicleVisualMesh, rearRightSpring.BoneName);
	pRearRightWheel->SetRelativeRotation(FRotator(0.f, -180.f, 0.f));

	/**
	 * Static meshes use special animator class of the component, that drives vehicle animations for the suspension and wheels.
	 * Skeletal meshes have their anim blueprint, so that's easier. We need to provide this class here.
	 * Let's set up our example class.
	 */
	MovementComponent->SetAnimatorClass(UStaticVehicleAnimatorExample::StaticClass());

	/* This is optional - set up some camera example. */
	UCameraComponent* pVehicleCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("VehicleCamera"));
	pVehicleCamera->SetupAttachment(RootComponent);
	pVehicleCamera->SetRelativeLocation(FVector(-640.f, 0.f, 270.f));
	pVehicleCamera->SetRelativeRotation(FRotator(-20.f, 0.f, 0.f));
}