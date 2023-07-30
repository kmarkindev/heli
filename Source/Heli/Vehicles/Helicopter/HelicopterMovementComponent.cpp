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

void UHelicopterMovementComponent::SetCollocation(float NewCollocation)
{
	CollocationData.CurrentCollocation = UKismetMathLibrary::FClamp(NewCollocation, 0.0, 1.0);
}

float UHelicopterMovementComponent::GetCollocation() const
{
	return CollocationData.CurrentCollocation;
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

void UHelicopterMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	FVector UpVector = GetOwner()->GetActorUpVector();
	
	FVector GravityVector = {0.f, 0.f, -1.f};
	float GravityAcceleration = 9.8f * 100.f;

	float CurrentCollocationAcceleration = UKismetMathLibrary::Lerp(
		CollocationData.MinCollocationAcceleration,
		CollocationData.MaxCollocationAcceleration,
		CollocationData.CurrentCollocation
	);

	FVector GravityAccelerationVector = GravityVector * GravityAcceleration;
	FVector CollocationAccelerationVector = UpVector * CurrentCollocationAcceleration;

	// Apply accelerations
	Velocity += (GravityAccelerationVector + CollocationAccelerationVector) * DeltaTime;

	// Apply damping
	Velocity *= FMath::Pow(DecelerationRate, DeltaTime);

	FVector OldLocation = GetOwner()->GetActorLocation();
	
	GetOwner()->AddActorWorldOffset(Velocity * DeltaTime, true);

	// Correct velocity based on actual traveled distance
	// in case of any collision
	FVector NewLocation = GetOwner()->GetActorLocation();
	Velocity = (NewLocation - OldLocation) / DeltaTime;
}

