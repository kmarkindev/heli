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
	
	CameraSpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(SpringArmComponentName);
	CameraSpringArmComponent->SetupAttachment(RootComponent);
	
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(CameraComponentName);
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
	if(HelicopterMeshComponent)
	{
		HelicopterMeshComponent->SetCollisionProfileName("Pawn", true);
		HelicopterMeshComponent->SetPhysMaterialOverride(HelicopterPhysicalMaterial);
	}
	else
	{
		HELI_ERR("Can't initialize helicopter %s because it has no skeletal mesh", *GetName());
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

void AHelicopter::SetAdditionalMass(float NewMass, bool bAddToCurrent)
{
	if(!HelicopterMovementComponent)
		return;
	
	return HelicopterMovementComponent->SetAdditionalMass(NewMass, bAddToCurrent);
}

float AHelicopter::GetAdditionalMass() const
{
	if(!HelicopterMovementComponent)
		return 0.f;
	
	return HelicopterMovementComponent->GetAdditionalMass();
}

float AHelicopter::GetRawMass() const
{
	if(!HelicopterMovementComponent)
		return 0.f;
	
	return HelicopterMovementComponent->GetRawMass();
}

float AHelicopter::GetActualMass() const
{
	if(!HelicopterMovementComponent)
		return 0.f;
	
	return HelicopterMovementComponent->GetActualMass();
}

float AHelicopter::GetVerticalSpeed() const
{
	return GetVelocity().Z;
}

float AHelicopter::GetHorizontalSpeed() const
{
	return GetVelocity().Size2D();
}

float AHelicopter::GetAltitude() const
{
	if(!HelicopterMovementComponent)
		return 0.f;

	return HelicopterMovementComponent->GetCurrentAltitude();
}

float AHelicopter::GetCurrentCollective() const
{
	if(!HelicopterMovementComponent)
		return 0.f;

	return HelicopterMovementComponent->GetCurrentCollective();
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

	const bool bSocketExists = HelicopterMeshComponent
		->DoesSocketExist(UHelicopterRootMeshComponent::HelicopterMeshSkeletonCameraSocketName);
	
	if(HelicopterMeshComponent && bSocketExists)
	{
		const FAttachmentTransformRules AttachRules {
			EAttachmentRule::SnapToTarget,
			EAttachmentRule::SnapToTarget,
			EAttachmentRule::SnapToTarget,
			true
		};
		
		CameraSpringArmComponent
			->AttachToComponent(
				RootComponent,
				AttachRules,
				UHelicopterRootMeshComponent::HelicopterMeshSkeletonCameraSocketName
			);
	}
	else
	{
		HELI_ERR("Can't attach camera spring arm since helicopter mesh doesn't have a socket to do that. "
		   "Spring arm remain attached to root component");
	}
}
