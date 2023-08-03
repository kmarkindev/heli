// Fill out your copyright notice in the Description page of Project Settings.


#include "Helicopter.h"

#include "CameraLookAroundComponent.h"
#include "HelicopterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

AHelicopter::AHelicopter()
{
	PrimaryActorTick.bCanEverTick = true;

	HelicopterMesh = CreateDefaultSubobject<USkeletalMeshComponent>(HelicopterMeshComponentName);
	SetRootComponent(HelicopterMesh);

	CameraSpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(CameraSpringArmComponent, USpringArmComponent::SocketName);

	HelicopterMovementComponent = CreateDefaultSubobject<UHelicopterMovementComponent>(HelicopterMovementComponentName);
	CameraLookAroundComponent = CreateDefaultSubobject<UCameraLookAroundComponent>(CameraLookAroundComponentName);
}

void AHelicopter::BeginPlay()
{
	Super::BeginPlay();
}

void AHelicopter::ConfigHelicopterMesh()
{
	HelicopterMesh->SetSimulatePhysics(true);
	HelicopterMesh->SetEnableGravity(true);

	HelicopterMesh->SetLinearDamping(0.2f);
	HelicopterMesh->SetAngularDamping(1.f);

	HelicopterMesh->SetCollisionProfileName("BlockAll", false);
}

void AHelicopter::ConfigSpringArmAndCamera()
{
	CameraSpringArmComponent->bUsePawnControlRotation = true;
	CameraSpringArmComponent->bInheritPitch = true;
	CameraSpringArmComponent->bInheritRoll = true;
	CameraSpringArmComponent->bInheritYaw = true;

	CameraSpringArmComponent->TargetArmLength = 0.f;
	
	CameraSpringArmComponent->bEnableCameraLag = false;
	CameraSpringArmComponent->bEnableCameraRotationLag = true;
	CameraSpringArmComponent->CameraRotationLagSpeed = 15.f;

	CameraComponent->SetFieldOfView(120.f);
}

void AHelicopter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AHelicopter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	ConfigHelicopterMesh();

	ConfigSpringArmAndCamera();
}

