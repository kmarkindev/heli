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

float UHelicopterMovementComponent::CalculateAccelerationAmountBasedOnCollective() const
{
	return UKismetMathLibrary::Lerp(
		PhysicsData.MinCollectiveAcceleration,
		PhysicsData.MaxCollectiveAcceleration,
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
	const float CurrentCollocationAccelerationAmount = CalculateAccelerationAmountBasedOnCollective();

	return AccelerationDirection * CurrentCollocationAccelerationAmount;
}

void UHelicopterMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateVelocity(DeltaTime);
}

void UHelicopterMovementComponent::ApplyScaleToResult(FVector& InOutResult, const FVector& DeltaToApply)
{
	// We do this since InOutResult axis might be negative together with DeltaToApply axis
	// in this case instead of scaling down or up, it will only scale it up exponentially
	
	InOutResult.X += InOutResult.X > 0  ? DeltaToApply.X : -DeltaToApply.X;
	InOutResult.Y += InOutResult.Y > 0  ? DeltaToApply.Y : -DeltaToApply.Y;
	InOutResult.Z += InOutResult.Z > 0  ? DeltaToApply.Z : -DeltaToApply.Z;
}

void UHelicopterMovementComponent::ApplyVelocityDamping(float DeltaTime)
{
	// Apply horizontal damping
	const float HorizontalDecelerationScale = PhysicsData.DecelerationScaleFromVelocityCurve
		? PhysicsData.DecelerationScaleFromVelocityCurve
			->GetFloatValue(USpeedConversionsLibrary::CmsToKmh(Velocity.Size2D()))
		: 1.f;
	
	Velocity += -Velocity.GetSafeNormal() * PhysicsData.HorizontalDeceleration
		* HorizontalDecelerationScale * DeltaTime;
	
	// Apply vertical damping
	const float VerticalDecelerationScale = PhysicsData.DecelerationScaleFromVelocityCurve
		? PhysicsData.DecelerationScaleFromVelocityCurve
			->GetFloatValue(USpeedConversionsLibrary::CmsToKmh(FMath::Abs(Velocity.Z)))
		: 1.f;
	
	Velocity.Z += -FMath::Sign(Velocity.Z) * PhysicsData.VerticalDeceleration
		* VerticalDecelerationScale * DeltaTime;
}

void UHelicopterMovementComponent::ClampVelocityToMaxSpeed()
{
	// Clamp horizontal speed
	Velocity = Velocity.GetClampedToMaxSize2D(PhysicsData.MaxHorizontalSpeed);
	
	// Clamp up speed
	Velocity.Z = FMath::Min(Velocity.Z, PhysicsData.MaxUpSpeed);

	// Clamp down speed
	Velocity.Z = FMath::Max(Velocity.Z, -PhysicsData.MaxDownSpeed);
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
	FVector CollocationAcceleration = CalculateCurrentCollectiveAccelerationVector();
	
	// Apply acceleration scales for each side

	// Forward
	const FVector ForwardVector = GetHorizontalForwardVector();
	ApplyAccelerationScaleAlongVector(
		CollocationAcceleration,
		AccelerationScales.ForwardAccelerationScale,
		AccelerationScales.ForwardAccelerationScaleDefault,
		ForwardVector
	);

	// Backward
	const FVector BackwardVector = -ForwardVector;
	ApplyAccelerationScaleAlongVector(
		CollocationAcceleration,
		AccelerationScales.BackwardAccelerationScale,
		AccelerationScales.BackwardAccelerationScaleDefault,
		BackwardVector
	);

	// Up
	const FVector UpVector = GetVerticalUpVector();
	ApplyAccelerationScaleAlongVector(
		CollocationAcceleration,
		AccelerationScales.UpAccelerationScale,
		AccelerationScales.UpAccelerationScaleDefault,
		UpVector
	);

	// Down
	const FVector DownVector = -UpVector;
	ApplyAccelerationScaleAlongVector(
		CollocationAcceleration,
		AccelerationScales.DownAccelerationScale,
		AccelerationScales.DownAccelerationScaleDefault,
		DownVector
	);

	// Right
	const FVector RightVector = GetHorizontalRightVector();
	ApplyAccelerationScaleAlongVector(
		CollocationAcceleration,
		AccelerationScales.SideAccelerationScale,
		AccelerationScales.SideAccelerationScaleDefault,
		RightVector
	);
	
	// Left
	const FVector LeftVector = -RightVector;
	ApplyAccelerationScaleAlongVector(
		CollocationAcceleration,
		AccelerationScales.SideAccelerationScale,
		AccelerationScales.SideAccelerationScaleDefault,
		LeftVector
	);
	
	Velocity += CollocationAcceleration * DeltaTime;
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
