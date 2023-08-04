// Fill out your copyright notice in the Description page of Project Settings.


#include "Helicopter.h"

#include "HelicopterMovementComponent.h"
#include "HelicopterRootMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Heli/LogHeli.h"
#include "Heli/Components/CameraLookAroundComponent.h"

AHelicopter::AHelicopter()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	PrimaryActorTick.EndTickGroup = TG_PrePhysics;

	HelicopterMeshComponent = CreateDefaultSubobject<UHelicopterRootMeshComponent>(HelicopterMeshComponentName);
	SetRootComponent(HelicopterMeshComponent);
	
	HelicopterMovementComponent = CreateDefaultSubobject<UHelicopterMovementComponent>(HelicopterMovementComponentName);
	
	CameraLookAroundComponent = CreateDefaultSubobject<UCameraLookAroundComponent>(CameraLookAroundComponentName);
	
	CameraSpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(SpringArmComponentName);

	if(HelicopterMeshComponent->DoesSocketExist(UHelicopterRootMeshComponent::HelicopterMeshSkeletonCameraSocketName))
	{
		CameraSpringArmComponent
			->SetupAttachment(RootComponent, UHelicopterRootMeshComponent::HelicopterMeshSkeletonCameraSocketName);
	}
	else
	{
		CameraSpringArmComponent->SetupAttachment(RootComponent);
		HELI_ERR("Can't attach camera spring arm since helicopter mesh doesn't have a socket to do that");
	}
	
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(CameraComponentName);
	CameraComponent->SetupAttachment(CameraSpringArmComponent, USpringArmComponent::SocketName);
}

void AHelicopter::BeginPlay()
{
	Super::BeginPlay();
}

void AHelicopter::ConfigHelicopterMesh()
{
	if(HelicopterMeshComponent)
	{
		HelicopterMeshComponent->SetCollisionProfileName("Pawn", false);
		HelicopterMeshComponent->SetSimulatePhysics(false);
	}
	else
	{
		HELI_ERR("Can't initialize helicopter %s because it has no skeletal mesh or physics asset", *GetName());
	}
}

void AHelicopter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AHelicopter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	ConfigHelicopterMesh();
	ConfigCameraAndSpringArm();
}

FVector AHelicopter::GetVelocity() const
{
	// Base Pawn::GetVelocity searches for UPawnMovement->Velocity or takes component vel only when physics sim enabled,
	// but we want to get velocity from component in both cases: with and without physics sim enabled
	
	return GetRootComponent()->GetComponentVelocity();
}

void AHelicopter::ConfigCameraAndSpringArm()
{
	if(!CameraSpringArmComponent || !CameraComponent)
	{
		HELI_ERR("Can't initialize camera and spring arm, some of them are null");
		return;
	}
	
	CameraSpringArmComponent->bUsePawnControlRotation = true;
	CameraSpringArmComponent->bInheritPitch = true;
	CameraSpringArmComponent->bInheritRoll = true;
	CameraSpringArmComponent->bInheritYaw = true;

	CameraSpringArmComponent->TargetArmLength = 0.f;

	CameraSpringArmComponent->bDoCollisionTest = false;
	
	CameraSpringArmComponent->bEnableCameraLag = false;
	CameraSpringArmComponent->bEnableCameraRotationLag = true;
	CameraSpringArmComponent->CameraRotationLagSpeed = 15.f;

	CameraComponent->SetFieldOfView(120.f);
}
