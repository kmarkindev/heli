// Fill out your copyright notice in the Description page of Project Settings.


#include "HelicopterMovementComponent.h"

#include "Heli/LogHeli.h"
#include "Kismet/KismetMathLibrary.h"

UHelicopterMovementComponent::UHelicopterMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	PrimaryComponentTick.EndTickGroup = TG_PrePhysics;
	
	bUpdateOnlyIfRendered = false;
	bAutoUpdateTickRegistration = true;
	bTickBeforeOwner = true;
	bAutoRegisterUpdatedComponent = true;
	bConstrainToPlane = false;
	bSnapToPlaneAtStart = false;
}

void UHelicopterMovementComponent::BeginPlay()
{
	Super::BeginPlay();
}

float UHelicopterMovementComponent::CalculateForceAmountBasedOnCollective() const
{
	return UKismetMathLibrary::Lerp(
		PhysicsData.MinLiftForce,
		PhysicsData.MaxLiftForce,
		CollectiveData.CurrentCollective
	);
}

void UHelicopterMovementComponent::SetCollective(float NewCollocation)
{
	CollectiveData.CurrentCollective = UKismetMathLibrary::FClamp(NewCollocation, 0.0, 1.0);
}

void UHelicopterMovementComponent::IncreaseCollective()
{
	const float DeltaTime = GetWorld()->DeltaTimeSeconds;
	
	SetCollective(CollectiveData.CurrentCollective + DeltaTime * CollectiveData.CollectiveIncreaseSpeed);
}

void UHelicopterMovementComponent::DecreaseCollective()
{
	const float DeltaTime = GetWorld()->DeltaTimeSeconds;

	SetCollective(CollectiveData.CurrentCollective - DeltaTime * CollectiveData.CollectiveDecreaseSpeed);
}

void UHelicopterMovementComponent::AddRotation(float PitchIntensity, float YawIntensity, float RollIntensity)
{
	PitchIntensity = UKismetMathLibrary::FClamp(PitchIntensity, -1.0, 1.0);
	RollIntensity = UKismetMathLibrary::FClamp(RollIntensity, -1.0, 1.0);
	YawIntensity = UKismetMathLibrary::FClamp(YawIntensity, -1.0, 1.0);

	const float DeltaTime = GetWorld()->DeltaTimeSeconds;
	
	FRotator Rotator {
		RotationData.PitchSpeed * PitchIntensity * DeltaTime,
		RotationData.YawSpeed * YawIntensity * DeltaTime,
		RotationData.RollSpeed * RollIntensity * DeltaTime
	};
	
	GetOwner()->AddActorLocalRotation(Rotator, true, nullptr, ETeleportType::TeleportPhysics);
}

FVector UHelicopterMovementComponent::CalculateCurrentCollectiveAccelerationVector() const
{
	const FVector AccelerationDirection = GetOwner()->GetActorUpVector();
	const float CurrentCollocationForceAmount = CalculateForceAmountBasedOnCollective();

	const float Acceleration = USpeedConversionsLibrary::MsToCms(CurrentCollocationForceAmount / GetActualMass());
	
	return AccelerationDirection * Acceleration;
}

float UHelicopterMovementComponent::GetGravityZ() const
{
	return PhysicsData.GravityZAcceleration;
}

float UHelicopterMovementComponent::GetMaxSpeed() const
{
	return PhysicsData.MaxSpeed;
}

float UHelicopterMovementComponent::GetActualMass() const
{
	return PhysicsData.MassKg + PhysicsData.AdditionalMassKg;
}

void UHelicopterMovementComponent::ApplyVelocityDamping(float DeltaTime)
{
	const float K = PhysicsData.AirFrictionDensityAndDrag * PhysicsData.AirFrictionAreaMSqr;
	const float AirFrictionForce = K * FMath::Square(Velocity.Length()) / 2;
	const float AirFrictionDeceleration = AirFrictionForce / GetActualMass();
	
	Velocity += -Velocity.GetSafeNormal() * AirFrictionDeceleration * DeltaTime;
}

void UHelicopterMovementComponent::ClampVelocityToMaxSpeed()
{
	Velocity = Velocity.GetClampedToMaxSize(PhysicsData.MaxSpeed);
}

void UHelicopterMovementComponent::ApplyVelocityToLocation(float DeltaTime)
{
	// Remember old location before move
	const FVector OldLocation = UpdatedComponent->GetComponentLocation();

	// Find values to use during move
	const FVector DeltaMove = Velocity * DeltaTime;
	const FQuat Rotation = UpdatedComponent->GetComponentQuat();

	if (DeltaMove.IsNearlyZero())
	{
		return;
	}
	
	// Perform move with collision test
	FHitResult Hit {};
	SafeMoveUpdatedComponent(DeltaMove, Rotation, true, Hit);
	
	// React on collision if any
	if (Hit.IsValidBlockingHit())
	{
		// react on collision if needed
		HandleImpact(Hit, DeltaTime, DeltaMove);
		
		// then try to slide the remaining distance along the surface
		SlideAlongSurface(DeltaMove, 1.f - Hit.Time, Hit.Normal, Hit, true);
	}
	
	// safe new location after collision
	const FVector NewLocation = UpdatedComponent->GetComponentLocation();

	Velocity = (NewLocation - OldLocation) / DeltaTime;
}

void UHelicopterMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateVelocity(DeltaTime);
}

void UHelicopterMovementComponent::UpdateVelocity(float DeltaTime)
{
	ApplyGravityToVelocity(DeltaTime);
	
	ApplyAccelerationsToVelocity(DeltaTime);

	ApplyVelocityDamping(DeltaTime);
	
	ClampVelocityToMaxSpeed();

	ApplyVelocityToLocation(DeltaTime);

	UpdateComponentVelocity();
}

void UHelicopterMovementComponent::ApplyGravityToVelocity(float DeltaTime)
{
	const FVector GravityAcceleration { 0.f, 0.f, GetGravityZ() };

	Velocity += GravityAcceleration * DeltaTime;
}

void UHelicopterMovementComponent::ApplyAccelerationsToVelocity(float DeltaTime)
{
	const FVector CollocationAcceleration = CalculateCurrentCollectiveAccelerationVector();
	
	Velocity += CollocationAcceleration * DeltaTime;
}