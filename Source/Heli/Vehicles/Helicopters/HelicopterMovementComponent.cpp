// Fill out your copyright notice in the Description page of Project Settings.


#include "HelicopterMovementComponent.h"

#include "Heli/LogHeli.h"
#include "Kismet/KismetMathLibrary.h"

UHelicopterMovementComponent::UHelicopterMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_DuringPhysics;
	PrimaryComponentTick.EndTickGroup = TG_DuringPhysics;
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
	const float DeltaTime = GetWorld()->DeltaTimeSeconds;
	
	SetCollocation(CollocationData.CurrentCollocation + DeltaTime * CollocationData.CollocationIncreaseSpeed);
}

void UHelicopterMovementComponent::DecreaseCollocation()
{
	const float DeltaTime = GetWorld()->DeltaTimeSeconds;

	SetCollocation(CollocationData.CurrentCollocation - DeltaTime * CollocationData.CollocationDecreaseSpeed);
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
	
	GetOwner()->AddActorLocalRotation(Rotator, true);
}

FVector UHelicopterMovementComponent::CalculateCurrentCollocationAccelerationVector() const
{
	const FVector UpVector = GetOwner()->GetActorUpVector();
	const float CurrentCollocationAccelerationAmount = CalculateAccelerationAmountBasedOnCollocation();

	return UpVector * CurrentCollocationAccelerationAmount;
}

void UHelicopterMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ApplyAccelerationsToVelocity(DeltaTime);
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
	const FVector ForwardVector = GetHorizontalForwardVector();
	ApplyAccelerationScaleAlongVector(
		CollocationAcceleration,
		PhysicsData.ForwardAccelerationScale,
		PhysicsData.ForwardAccelerationScaleDefault,
		ForwardVector
	);

	// Backward
	const FVector BackwardVector = -ForwardVector;
	ApplyAccelerationScaleAlongVector(
		CollocationAcceleration,
		PhysicsData.BackwardAccelerationScale,
		PhysicsData.BackwardAccelerationScaleDefault,
		BackwardVector
	);

	// Up
	const FVector UpVector = GetVerticalUpVector();
	ApplyAccelerationScaleAlongVector(
		CollocationAcceleration,
		PhysicsData.UpAccelerationScale,
		PhysicsData.UpAccelerationScaleDefault,
		UpVector
	);

	// Down
	const FVector DownVector = -UpVector;
	ApplyAccelerationScaleAlongVector(
		CollocationAcceleration,
		PhysicsData.DownAccelerationScale,
		PhysicsData.DownAccelerationScaleDefault,
		DownVector
	);

	// Right
	const FVector RightVector = GetHorizontalRightVector();
	ApplyAccelerationScaleAlongVector(
		CollocationAcceleration,
		PhysicsData.SideAccelerationScale,
		PhysicsData.SideAccelerationScaleDefault,
		RightVector
	);
	
	// Left
	const FVector LeftVector = -RightVector;
	ApplyAccelerationScaleAlongVector(
		CollocationAcceleration,
		PhysicsData.SideAccelerationScale,
		PhysicsData.SideAccelerationScaleDefault,
		LeftVector
	);
	
	UPrimitiveComponent* const RootComponent = GetRootPrimitiveComponent();

	FVector LinearVelocity = RootComponent->GetPhysicsLinearVelocity();
	
	LinearVelocity += CollocationAcceleration * DeltaTime;
	LinearVelocity = LinearVelocity.GetClampedToMaxSize(PhysicsData.MaxSpeed);
	
	RootComponent->SetPhysicsLinearVelocity(LinearVelocity);
}

void UHelicopterMovementComponent::ApplyAccelerationScaleAlongVector(FVector& BaseAcceleration,
	const UCurveFloat* ScaleCurve, float DefaultScale, const FVector& ScaleDirectionNormalized)
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

	// Get current acceleration scale on current speed
	const float Scale = ScaleCurve
		? ScaleCurve->GetFloatValue(USpeedConversionsLibrary::CmsToKmh(BaseAccelerationAlongDirection.Length()))
		: DefaultScale;
	
	// Scale it
	const FVector ScaledAccelerationAlongDirection = BaseAccelerationAlongDirection * Scale;

	// Add scaled acceleration
	const FVector AccelerationDelta = ScaledAccelerationAlongDirection - BaseAccelerationAlongDirection;
	ApplyScaleToResult(BaseAcceleration, AccelerationDelta);
}

FVector UHelicopterMovementComponent::GetHorizontalForwardVector() const
{
	FVector ForwardVector = GetOwner()->GetActorForwardVector();
	ForwardVector.Z = 0.f;
	ForwardVector.Normalize();

	return ForwardVector;
}

FVector UHelicopterMovementComponent::GetHorizontalRightVector() const
{
	FVector RightVector = GetOwner()->GetActorRightVector();
	RightVector.Z = 0.f;
	RightVector.Normalize();

	return RightVector;
}

FVector UHelicopterMovementComponent::GetVerticalUpVector() const
{
	return {0.f, 0.f, 1.f};
}

UPrimitiveComponent* UHelicopterMovementComponent::GetRootPrimitiveComponent() const
{
	UPrimitiveComponent* const RootComponent = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent());

	if(!RootComponent)
	{
		HELI_ERR("Can't get root component since it's not primitive component type");
		return nullptr;
	}

	return RootComponent;
}
