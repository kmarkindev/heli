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
	
	const FRotator Rotator {
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
	// We use raw accelerations since air friction doesn't depend on helicopter mass
	// and we don't want to make all of these too complicated
	
	// Apply horizontal air friction
	// Note: Horizontal Speed is always positive
	const float HorizontalSpeed = USpeedConversionsLibrary::CmsToKmh(Velocity.Size2D());
	const float HorizontalAirFrictionDeceleration = PhysicsData.HorizontalAirFrictionDecelerationToVelocityCurve
		? USpeedConversionsLibrary::KmhToCms(
			PhysicsData.HorizontalAirFrictionDecelerationToVelocityCurve->GetFloatValue(HorizontalSpeed)
		)
		: 0.f;
	
	Velocity += -Velocity.GetSafeNormal2D() * HorizontalAirFrictionDeceleration * DeltaTime;

	// Apply vertical air friction
	// Note: Vertical Speed may be negative (in case of falling)
	const float VerticalSpeed = USpeedConversionsLibrary::CmsToKmh(Velocity.Z);
	const float VerticalAirFrictionDeceleration = PhysicsData.VerticalAirFrictionDecelerationToVelocityCurve
		? USpeedConversionsLibrary::KmhToCms(
			PhysicsData.VerticalAirFrictionDecelerationToVelocityCurve->GetFloatValue(VerticalSpeed)
		)
		: 0.f;

	Velocity.Z += -FMath::Sign(Velocity.Z) * VerticalAirFrictionDeceleration * DeltaTime;
}

void UHelicopterMovementComponent::ClampVelocityToMaxSpeed()
{
	const float AverageMaxSpeed = GetMaxSpeed() * PhysicsData.AverageMaxSpeedScale;
	
	// Allow helicopter to fall faster then anything
	Velocity.Z = FMath::Clamp(Velocity.Z, -PhysicsData.MaxSpeed, AverageMaxSpeed);
	
	// Limit horizontal velocity
	const FVector ClampedHorizontal = Velocity.GetClampedToMaxSize2D(AverageMaxSpeed);
	Velocity.X = ClampedHorizontal.X;
	Velocity.Y = ClampedHorizontal.Y;
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
	ApplyAccelerationsToVelocity(DeltaTime);
	
	ApplyGravityToVelocity(DeltaTime);
	
	ClampVelocityToMaxSpeed();

	ApplyVelocityToLocation(DeltaTime);

	ApplyVelocityDamping(DeltaTime);
	
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