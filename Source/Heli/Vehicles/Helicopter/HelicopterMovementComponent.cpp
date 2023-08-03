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

void UHelicopterMovementComponent::ApplyLinearMovement(float DeltaTime)
{
	const FVector CollocationAcceleration = CalculateCurrentCollocationAccelerationVector();
	UPrimitiveComponent* Comp = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent());

	FVector LinearVelocity = Comp->GetPhysicsLinearVelocity();
	
	LinearVelocity += CollocationAcceleration * DeltaTime;

	LinearVelocity = LinearVelocity.GetClampedToMaxSize(PhysicsData.MaxSpeed);
	
	Comp->SetPhysicsLinearVelocity(LinearVelocity);
}

void UHelicopterMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ApplyLinearMovement(DeltaTime);
}
