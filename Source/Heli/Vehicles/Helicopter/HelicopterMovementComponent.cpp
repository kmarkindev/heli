// Fill out your copyright notice in the Description page of Project Settings.


#include "HelicopterMovementComponent.h"

#include "Kismet/KismetMathLibrary.h"

UHelicopterMovementComponent::UHelicopterMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}

void UHelicopterMovementComponent::BeginPlay()
{
	Super::BeginPlay();
}

float UHelicopterMovementComponent::CalculateAccelerationAmountBasedOnCollocation() const
{
	return UKismetMathLibrary::Lerp(
		PhysicsData.MinCollocationAcceleration,
		PhysicsData.MaxCollocationAcceleration,
		CollocationData.CurrentCollocation
	);
}

void UHelicopterMovementComponent::SetCollocation(float NewCollocation)
{
	CollocationData.CurrentCollocation = UKismetMathLibrary::FClamp(NewCollocation, 0.0, 1.0);
}

void UHelicopterMovementComponent::IncreaseCollocation()
{
	float DeltaTime = GetWorld()->DeltaTimeSeconds;
	
	SetCollocation(CollocationData.CurrentCollocation + DeltaTime * CollocationData.CollocationIncreaseSpeed);
}

void UHelicopterMovementComponent::DecreaseCollocation()
{
	float DeltaTime = GetWorld()->DeltaTimeSeconds;

	SetCollocation(CollocationData.CurrentCollocation - DeltaTime * CollocationData.CollocationDecreaseSpeed);
}

void UHelicopterMovementComponent::AddRotation(float PitchIntensity, float YawIntensity, float RollIntensity)
{
	PitchIntensity = UKismetMathLibrary::FClamp(PitchIntensity, -1.0, 1.0);
	RollIntensity = UKismetMathLibrary::FClamp(RollIntensity, -1.0, 1.0);
	YawIntensity = UKismetMathLibrary::FClamp(YawIntensity, -1.0, 1.0);

	float DeltaTime = GetWorld()->DeltaTimeSeconds;
	
	FRotator Rotator {
		RotationData.PitchSpeed * PitchIntensity * DeltaTime,
		RotationData.YawSpeed * YawIntensity * DeltaTime,
		RotationData.RollSpeed * RollIntensity * DeltaTime
	};
	
	GetOwner()->AddActorLocalRotation(Rotator, true);
}

FVector UHelicopterMovementComponent::CalculateCurrentCollocationAccelerationVector() const
{
	const FVector UpVector = GetOwner()->GetActorUpVector();
	const float CurrentCollocationAccelerationAmount = CalculateAccelerationAmountBasedOnCollocation();

	return UpVector * CurrentCollocationAccelerationAmount;
}

void UHelicopterMovementComponent::ApplyScaleToResult(FVector& InOutResult, const FVector& DeltaToApply)
{
	// We do this since InOutResult axis might be negative together with DeltaToApply axis
	// in this case instead of scaling down or up, it will only scale it up exponentially
	
	InOutResult.X += InOutResult.X > 0  ? DeltaToApply.X : -DeltaToApply.X;
	InOutResult.Y += InOutResult.Y > 0  ? DeltaToApply.Y : -DeltaToApply.Y;
	InOutResult.Z += InOutResult.Z > 0  ? DeltaToApply.Z : -DeltaToApply.Z;
}

void UHelicopterMovementComponent::ApplyAccelerationsToVelocity(float DeltaTime)
{
	FVector CollocationAcceleration = CalculateCurrentCollocationAccelerationVector();

	// Apply acceleration scales for each side

	// Forward
	const FVector ForwardVector = GetOwner()->GetActorForwardVector();
	ApplyAccelerationScaleAlongVector(CollocationAcceleration,
		PhysicsData.ForwardAccelerationScale, ForwardVector);

	// Backward
	const FVector BackwardVector = -ForwardVector;
	ApplyAccelerationScaleAlongVector(CollocationAcceleration,
		PhysicsData.BackwardAccelerationScale, BackwardVector);

	// Up
	const FVector UpVector = GetOwner()->GetActorUpVector();
	ApplyAccelerationScaleAlongVector(CollocationAcceleration,
		PhysicsData.UpAccelerationScale, UpVector);

	// Down
	const FVector DownVector = -UpVector;
	ApplyAccelerationScaleAlongVector(CollocationAcceleration,
		PhysicsData.DownAccelerationScale, DownVector);

	// Right
	const FVector RightVector = GetOwner()->GetActorRightVector();
	ApplyAccelerationScaleAlongVector(CollocationAcceleration,
		PhysicsData.SideAccelerationScale, RightVector);
	
	// Left
	const FVector LeftVector = -RightVector;
	ApplyAccelerationScaleAlongVector(CollocationAcceleration,
		PhysicsData.SideAccelerationScale, LeftVector);

	// Finally apply acceleration
	PhysicsData.Velocity += (PhysicsData.GravityAcceleration + CollocationAcceleration) * DeltaTime;
}

void UHelicopterMovementComponent::ApplyAccelerationScaleAlongVector(FVector& BaseAcceleration, float Scale,
	const FVector& ScaleDirectionNormalized)
{
	// Get acceleration along forward vector
	FVector BaseAccelerationAlongDirection = BaseAcceleration * ScaleDirectionNormalized;

	// We don't want to scale negative acceleration
	// e.g. when applying forward and backward acceleration scales,
	// without this we will scale both forward and backward accelerations,
	// by applying both forward and backward scales to same acceleration
	BaseAccelerationAlongDirection.X = FMath::Max(BaseAccelerationAlongDirection.X, 0.f);
	BaseAccelerationAlongDirection.Y = FMath::Max(BaseAccelerationAlongDirection.Y, 0.f);
	BaseAccelerationAlongDirection.Z = FMath::Max(BaseAccelerationAlongDirection.Z, 0.f);

	// Scale it
	const FVector ScaledAccelerationAlongDirection = BaseAccelerationAlongDirection * Scale;

	// Add scaled acceleration
	const FVector AccelerationDelta = ScaledAccelerationAlongDirection - BaseAccelerationAlongDirection;
	ApplyScaleToResult(BaseAcceleration, AccelerationDelta);
}

void UHelicopterMovementComponent::ApplyDampingToVelocity(float DeltaTime)
{
	// Forward
	const FVector ForwardVector = GetOwner()->GetActorForwardVector();
	ApplyDampingAlongVector(DeltaTime, PhysicsData.ForwardDecelerationRate, ForwardVector);

	// Backward
	const FVector BackwardVector = -ForwardVector;
	ApplyDampingAlongVector(DeltaTime, PhysicsData.BackwardDecelerationRate, BackwardVector);

	// Up
	const FVector UpVector = GetOwner()->GetActorUpVector();
	ApplyDampingAlongVector(DeltaTime, PhysicsData.UpDecelerationRate, UpVector);

	// Down
	const FVector DownVector = -UpVector;
	ApplyDampingAlongVector(DeltaTime, PhysicsData.DownDecelerationRate, DownVector);

	// Right
	const FVector RightVector = GetOwner()->GetActorRightVector();
	ApplyDampingAlongVector(DeltaTime, PhysicsData.SideDecelerationRate, RightVector);
	
	// Left
	const FVector LeftVector = -RightVector;
	ApplyDampingAlongVector(DeltaTime, PhysicsData.SideDecelerationRate, LeftVector);
}

void UHelicopterMovementComponent::ApplyDampingAlongVector(float DeltaTime, float DampingRate,
	const FVector& DampingDirectionNormalized)
{
	// Find velocity along direction
	FVector BaseDirectionVelocity = PhysicsData.Velocity * DampingDirectionNormalized;
	
	// Do not damp negative velocity for the same reason as for acceleration
	BaseDirectionVelocity.X = FMath::Max(BaseDirectionVelocity.X, 0.f);
	BaseDirectionVelocity.Y = FMath::Max(BaseDirectionVelocity.Y, 0.f);
	BaseDirectionVelocity.Z = FMath::Max(BaseDirectionVelocity.Z, 0.f);
	
	// damp it
	const FVector DampedDirectionVelocity = BaseDirectionVelocity * FMath::Pow(DampingRate, DeltaTime);

	// Add damped velocity
	const FVector VelocityDelta = DampedDirectionVelocity - BaseDirectionVelocity;
	ApplyScaleToResult(PhysicsData.Velocity, VelocityDelta);
}

void UHelicopterMovementComponent::ApplyVelocityToLocation(float DeltaTime, FVector& OutOldLocation, FVector& OutNewLocation)
{
	OutOldLocation = GetOwner()->GetActorLocation();
	
	GetOwner()->AddActorWorldOffset(PhysicsData.Velocity * DeltaTime, true);

	OutNewLocation = GetOwner()->GetActorLocation();
}

void UHelicopterMovementComponent::RecalculateVelocityBasedOnTraveledDistance(float DeltaTime,
	const FVector& OldLocation, const FVector& NewLocation)
{
	// Correct velocity based on actual traveled distance in case of any collision or any other outer factors
	PhysicsData.Velocity = (NewLocation - OldLocation) / DeltaTime;
}

void UHelicopterMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	ApplyAccelerationsToVelocity(DeltaTime);
	
	ApplyDampingToVelocity(DeltaTime);

	FVector OldLocation {};
	FVector NewLocation {};
	ApplyVelocityToLocation(DeltaTime, OldLocation, NewLocation);

	RecalculateVelocityBasedOnTraveledDistance(DeltaTime, OldLocation, NewLocation);
}

