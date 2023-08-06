// Fill out your copyright notice in the Description page of Project Settings.


#include "HelicopterMovementComponent.h"
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
	// Get collective lift force scale from curve
	// or if there is no curve, use collective as a scale itself
	
	const float LiftForceScale = PhysicsData.LiftForceScaleFromCollectiveCurve
		? PhysicsData.LiftForceScaleFromCollectiveCurve->GetFloatValue(CollectiveData.CurrentCollective)
		: CollectiveData.CurrentCollective;

	return PhysicsData.LiftForceFromMaxCollective * LiftForceScale;
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
	// Allow to collect input from different source, but do not allow it to be more than possible
	
	RotationData.PitchPending = FMath::Clamp(RotationData.PitchPending + PitchIntensity, -1.f, 1.f);
	RotationData.RollPending = FMath::Clamp(RotationData.RollPending + RollIntensity, -1.f, 1.f);
	RotationData.YawPending = FMath::Clamp(RotationData.YawPending + YawIntensity, -1.f, 1.f);
}

FVector UHelicopterMovementComponent::CalculateCurrentCollectiveAccelerationVector() const
{
	const FVector AccelerationDirection = GetOwner()->GetActorUpVector();
	const float CurrentCollocationForceAmount = CalculateForceAmountBasedOnCollective();
	
	const float Acceleration = USpeedConversionsLibrary::MsToCms(CurrentCollocationForceAmount / GetActualMass());
	
	FVector FinalAcceleration = AccelerationDirection * Acceleration;
	
	float LiftScaleFromRotation = PhysicsData.LiftScaleFromRotationCurve
		? PhysicsData.LiftScaleFromRotationCurve->GetFloatValue(GetCurrentAngle())
		: 1.f;
	LiftScaleFromRotation = FMath::Clamp(LiftScaleFromRotation, 0.f, 1.f);

	// Do not scale lift when going to the ground
	FinalAcceleration.Z = FMath::Min(FinalAcceleration.Z, FinalAcceleration.Z * LiftScaleFromRotation);
	
	return FinalAcceleration;
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

void UHelicopterMovementComponent::ApplyVelocitiesToLocation(float DeltaTime)
{
	// Remember old location before move
	const FVector OldLocation = UpdatedComponent->GetComponentLocation();

	// Find values to use during move
	const FVector DeltaMove = Velocity * DeltaTime;
	FRotator Rotation = UpdatedComponent->GetComponentRotation();

	// Apply angular velocity
	if(!RotationData.AngularVelocity.IsNearlyZero())
	{
		const FRotator PitchRotator = UKismetMathLibrary::RotatorFromAxisAndAngle(
			UpdatedComponent->GetRightVector(), RotationData.AngularVelocity.X * DeltaTime
		);
		const FRotator RollRotator = UKismetMathLibrary::RotatorFromAxisAndAngle(
			UpdatedComponent->GetForwardVector(), RotationData.AngularVelocity.Y * DeltaTime
		);
		const FRotator YawRotator = UKismetMathLibrary::RotatorFromAxisAndAngle(
			UpdatedComponent->GetUpVector(), RotationData.AngularVelocity.Z * DeltaTime
		);
		
		Rotation = UKismetMathLibrary::ComposeRotators(Rotation, RollRotator);
		Rotation = UKismetMathLibrary::ComposeRotators(Rotation, PitchRotator);
		Rotation = UKismetMathLibrary::ComposeRotators(Rotation, YawRotator);
	}

	// Apply linear velocity
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

float UHelicopterMovementComponent::GetCurrentAngle() const
{
	const FVector UpVector {0.f, 0.f ,1.f};
	const FVector ComponentUpVector = UpdatedComponent->GetUpVector();

	const float DotProduct = UpVector.Dot(ComponentUpVector);
	const float AngleDecrees = FMath::RadiansToDegrees(FMath::Acos(DotProduct));
	
	return AngleDecrees;
}

void UHelicopterMovementComponent::UpdateAngularVelocity(float DeltaTime)
{
	const bool bHasMoved = ApplyAccelerationsToAngularVelocity(DeltaTime);

	// Do not apply deceleration if we rotated
	// It allows to rotate even with low (0.1) intensity
	if(!bHasMoved)
	{
		ApplyAngularVelocityDamping(DeltaTime);
	}

	ClampAngularVelocity();
}

bool UHelicopterMovementComponent::ApplyAccelerationsToAngularVelocity(float DeltaTime)
{
	const float PitchAcceleration = RotationData.PitchPending * RotationData.PitchAcceleration * DeltaTime;
	const float RollAcceleration = RotationData.RollPending * RotationData.RollAcceleration * DeltaTime;
	const float YawAcceleration = RotationData.YawPending * RotationData.YawAcceleration * DeltaTime;

	const FVector Delta = {
		PitchAcceleration,
		RollAcceleration,
		YawAcceleration
	};

	RotationData.AngularVelocity += Delta;

	RotationData.PitchPending = 0.f;
	RotationData.RollPending = 0.f;
	RotationData.YawPending = 0.f;
	
	return !Delta.IsNearlyZero();
}

void UHelicopterMovementComponent::ApplyAngularVelocityDamping(float DeltaTime)
{
	// Find new pitch, roll, yaw

	const int PitchSign = FMath::Sign(RotationData.AngularVelocity.X); 
	const float Pitch = RotationData.AngularVelocity.X
		+ -PitchSign
		* RotationData.PitchDeceleration
		* DeltaTime;

	const int RollSign = FMath::Sign(RotationData.AngularVelocity.Y);
	const float Roll = RotationData.AngularVelocity.Y
		+ -RollSign
		* RotationData.RollDeceleration
		* DeltaTime;

	const int YawSign = FMath::Sign(RotationData.AngularVelocity.Z);
	const float Yaw = RotationData.AngularVelocity.Z
		+ -YawSign
		* RotationData.YawDeceleration
		* DeltaTime;
	
	// clamp them so they dont produce opposite acceleration and save
	RotationData.AngularVelocity.X = PitchSign == 1
		? FMath::Max(0.f, Pitch)
		: FMath::Min(0.f, Pitch);

	RotationData.AngularVelocity.Y = RollSign == 1
		? FMath::Max(0.f, Roll)
		: FMath::Min(0.f, Roll);

	RotationData.AngularVelocity.Z = YawSign == 1
		? FMath::Max(0.f, Yaw)
		: FMath::Min(0.f, Yaw);
}

void UHelicopterMovementComponent::ClampAngularVelocity()
{
	RotationData.AngularVelocity.X = FMath::ClampAngle(
		RotationData.AngularVelocity.X,
		-RotationData.PitchMaxSpeed,
		RotationData.PitchMaxSpeed
	);

	RotationData.AngularVelocity.Y = FMath::ClampAngle(
		RotationData.AngularVelocity.Y,
		-RotationData.RollMaxSpeed,
		RotationData.RollMaxSpeed
	);

	RotationData.AngularVelocity.Z = FMath::ClampAngle(
		RotationData.AngularVelocity.Z,
		-RotationData.YawMaxSpeed,
		RotationData.YawMaxSpeed
	);
}

void UHelicopterMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateVelocity(DeltaTime);

	UpdateAngularVelocity(DeltaTime);

	ApplyVelocitiesToLocation(DeltaTime);
}

void UHelicopterMovementComponent::UpdateVelocity(float DeltaTime)
{
	ApplyAccelerationsToVelocity(DeltaTime);
	
	ApplyGravityToVelocity(DeltaTime);
	
	ClampVelocityToMaxSpeed();

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