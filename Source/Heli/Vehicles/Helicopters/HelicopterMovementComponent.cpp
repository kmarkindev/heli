// Fill out your copyright notice in the Description page of Project Settings.


#include "HelicopterMovementComponent.h"

#include "Heli/LogHeli.h"
#include "Heli/BFLs/HeliMathLibrary.h"
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

void UHelicopterMovementComponent::UpdateComponentVelocity()
{
	if(UpdatedPrimitive)
	{
		Velocity = UpdatedPrimitive->GetComponentVelocity();
	}
	
	Super::UpdateComponentVelocity();
}

void UHelicopterMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();

	if(UpdatedPrimitive && UpdatedPrimitive->CanEditSimulatePhysics())
	{
		UpdatedPrimitive->SetSimulatePhysics(true);
		UpdatedPrimitive->SetEnableGravity(false);
		ToggleDefaultDamping(false);
	}
	else
	{
		HELI_LOG("Can't config physics on UpdatedPrimitive");
	}
}

void UHelicopterMovementComponent::ApplyVelocityDamping(float DeltaTime)
{
	if(!UpdatedPrimitive)
		return;

	FVector PhysicsVelocity = UpdatedPrimitive->GetPhysicsLinearVelocity();
	
	// We use raw accelerations since air friction doesn't depend on helicopter mass
	// and we don't want to make all of these too complicated
	
	// Apply horizontal air friction
	// Note: Horizontal Speed is always positive
	const float HorizontalSpeed = USpeedConversionsLibrary::CmsToKmh(PhysicsVelocity.Size2D());
	const float HorizontalAirFrictionDeceleration = PhysicsData.HorizontalAirFrictionDecelerationToVelocityCurve
		? USpeedConversionsLibrary::KmhToCms(
			PhysicsData.HorizontalAirFrictionDecelerationToVelocityCurve->GetFloatValue(HorizontalSpeed)
		)
		: 0.f;
	
	PhysicsVelocity += -PhysicsVelocity.GetSafeNormal2D() * HorizontalAirFrictionDeceleration * DeltaTime;

	// Apply vertical air friction
	// Note: Vertical Speed may be negative (in case of falling)
	const float VerticalSpeed = USpeedConversionsLibrary::CmsToKmh(PhysicsVelocity.Z);
	const float VerticalAirFrictionDeceleration = PhysicsData.VerticalAirFrictionDecelerationToVelocityCurve
		? USpeedConversionsLibrary::KmhToCms(
			PhysicsData.VerticalAirFrictionDecelerationToVelocityCurve->GetFloatValue(VerticalSpeed)
		)
		: 0.f;

	PhysicsVelocity.Z += -FMath::Sign(PhysicsVelocity.Z) * VerticalAirFrictionDeceleration * DeltaTime;

	UpdatedPrimitive->SetPhysicsLinearVelocity(PhysicsVelocity);
}

void UHelicopterMovementComponent::ClampVelocityToMaxSpeed()
{
	if(!UpdatedPrimitive)
		return;
	
	const float AverageMaxSpeed = GetMaxSpeed() * PhysicsData.AverageMaxSpeedScale;

	FVector PhysicsVelocity = UpdatedPrimitive->GetPhysicsLinearVelocity();
	
	// Allow helicopter to fall faster then anything
	PhysicsVelocity.Z = FMath::Clamp(PhysicsVelocity.Z, -PhysicsData.MaxSpeed, AverageMaxSpeed);
	
	// Limit horizontal velocity
	const FVector ClampedHorizontal = PhysicsVelocity.GetClampedToMaxSize2D(AverageMaxSpeed);
	PhysicsVelocity.X = ClampedHorizontal.X;
	PhysicsVelocity.Y = ClampedHorizontal.Y;

	UpdatedPrimitive->SetPhysicsLinearVelocity(PhysicsVelocity);
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
	if(!UpdatedPrimitive)
		return false;
	
	const float PitchAcceleration = RotationData.PitchPending * RotationData.PitchAcceleration * DeltaTime;
	const float RollAcceleration = RotationData.RollPending * RotationData.RollAcceleration * DeltaTime;
	const float YawAcceleration = RotationData.YawPending * RotationData.YawAcceleration * DeltaTime;
	
	// Create local rotation

	FRotator RotationToApply {};
	
	// Apply pitch
	RotationToApply = UKismetMathLibrary::ComposeRotators(
		RotationToApply,
		UKismetMathLibrary::RotatorFromAxisAndAngle(
			UpdatedPrimitive->GetRightVector(),
			PitchAcceleration
		)
	);

	// Apply roll
	RotationToApply = UKismetMathLibrary::ComposeRotators(
		RotationToApply,
		UKismetMathLibrary::RotatorFromAxisAndAngle(
			UpdatedPrimitive->GetForwardVector(),
			RollAcceleration
		)
	);
	
	// Apply yaw
	RotationToApply = UKismetMathLibrary::ComposeRotators(
		RotationToApply,
		UKismetMathLibrary::RotatorFromAxisAndAngle(
			UpdatedPrimitive->GetUpVector(),
			YawAcceleration
		)
	);
	
	const FVector Delta = RotationToApply.Euler();

	const bool bMoved = !Delta.IsNearlyZero();

	// Do not touch velocity if we don't really need to
	if(bMoved)
	{
		UpdatedPrimitive->SetPhysicsAngularVelocityInDegrees(Delta, true);
	}
	
	RotationData.PitchPending = 0.f;
	RotationData.RollPending = 0.f;
	RotationData.YawPending = 0.f;
	
	return bMoved;
}

void UHelicopterMovementComponent::ApplyAngularVelocityDamping(float DeltaTime)
{
	if(!UpdatedPrimitive)
		return;

	FRotator PhysicsAngularVelocity = FRotator::MakeFromEuler(UpdatedPrimitive->GetPhysicsAngularVelocityInDegrees());

	auto Lambda = [&](const FVector& Axis, float Deceleration)
	{
		float AxisVelocity = UHeliMathLibrary::GetRotationAroundAxis(
			PhysicsAngularVelocity,
			Axis
		);
		
		const int YawSign = FMath::Sign(AxisVelocity);
		AxisVelocity += -YawSign
			* Deceleration
			* DeltaTime;
		
		UHeliMathLibrary::SetRotationAroundAxis(
			PhysicsAngularVelocity,
			Axis,
			AxisVelocity
		);
	};

	Lambda(
	UpdatedPrimitive->GetRightVector(),
		RotationData.PitchDeceleration
	);

	Lambda(
	UpdatedPrimitive->GetForwardVector(),
		RotationData.RollDeceleration
	);

	Lambda(
	UpdatedPrimitive->GetUpVector(),
		RotationData.YawDeceleration
	);
	
	UpdatedPrimitive->SetPhysicsAngularVelocityInDegrees(PhysicsAngularVelocity.Euler());
}

void UHelicopterMovementComponent::ClampAngularVelocity()
{
	if(!UpdatedPrimitive)
		return;

	FRotator PhysicsAngularVelocity = FRotator::MakeFromEuler(UpdatedPrimitive->GetPhysicsAngularVelocityInDegrees());

	UHeliMathLibrary::ClampVelocityAroundAxis(
		PhysicsAngularVelocity,
		UpdatedPrimitive->GetRightVector(),
		-RotationData.PitchMaxSpeed,
		RotationData.PitchMaxSpeed
	);

	UHeliMathLibrary::ClampVelocityAroundAxis(
		PhysicsAngularVelocity,
		UpdatedPrimitive->GetForwardVector(),
		-RotationData.RollMaxSpeed,
		RotationData.RollMaxSpeed
	);
	
	UHeliMathLibrary::ClampVelocityAroundAxis(
		PhysicsAngularVelocity,
		UpdatedPrimitive->GetUpVector(),
		-RotationData.YawMaxSpeed,
		RotationData.YawMaxSpeed
	);
	
	UpdatedPrimitive->SetPhysicsAngularVelocityInDegrees(PhysicsAngularVelocity.Euler());
}

void UHelicopterMovementComponent::ToggleDefaultDamping(bool bEnable)
{
	if(UpdatedPrimitive)
	{
		if(!bEnable)
		{
			UpdatedPrimitive->SetLinearDamping(0.f);
			UpdatedPrimitive->SetAngularDamping(0.f);
		}
		else
		{
			UpdatedPrimitive->SetLinearDamping(PhysicsData.DefaultLinearDamping);
			UpdatedPrimitive->SetAngularDamping(PhysicsData.DefaultAngularDamping);
		}

		PhysicsData.bIsDefaultDampingEnabled = bEnable;
	}
}

void UHelicopterMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateVelocity(DeltaTime);

	UpdateAngularVelocity(DeltaTime);
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
	if(!UpdatedPrimitive)
		return;
	
	const FVector GravityAcceleration { 0.f, 0.f, GetGravityZ() };

	UpdatedPrimitive->SetPhysicsLinearVelocity(GravityAcceleration * DeltaTime, true);
}

void UHelicopterMovementComponent::ApplyAccelerationsToVelocity(float DeltaTime)
{
	if(!UpdatedPrimitive)
		return;
	
	const FVector CollocationAcceleration = CalculateCurrentCollectiveAccelerationVector();

	UpdatedPrimitive->SetPhysicsLinearVelocity(CollocationAcceleration * DeltaTime, true);
}