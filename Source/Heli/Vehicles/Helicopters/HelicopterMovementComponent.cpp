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

float UHelicopterMovementComponent::GetRawMass() const
{
	return PhysicsData.MassKg;
}

float UHelicopterMovementComponent::GetActualMass() const
{
	return PhysicsData.MassKg + PhysicsData.AdditionalMassKg;
}

float UHelicopterMovementComponent::GetAdditionalMass() const
{
	return PhysicsData.AdditionalMassKg;
}

void UHelicopterMovementComponent::SetAdditionalMass(float NewMass, bool bAddToCurrent)
{
	if(bAddToCurrent)
		NewMass += PhysicsData.AdditionalMassKg;

	PhysicsData.AdditionalMassKg = FMath::Clamp(NewMass, 0.f, PhysicsData.MaxAdditionalMassKg);

	SyncPhysicsAndComponentMass();
}

float UHelicopterMovementComponent::GetCurrentCollective() const
{
	return CollectiveData.CurrentCollective;
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
		UpdatedPrimitive->SetLinearDamping(0.f);
		UpdatedPrimitive->SetAngularDamping(0.01f);

		SyncPhysicsAndComponentMass();
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
	
	FVector Delta {
		RotationData.RollPending * RotationData.RollAcceleration * DeltaTime,
		RotationData.PitchPending * RotationData.PitchAcceleration * DeltaTime,
		RotationData.YawPending * RotationData.YawAcceleration * DeltaTime
	};

	const FTransform ComponentTransform = UpdatedPrimitive->GetComponentTransform();
	Delta = UKismetMathLibrary::TransformDirection(ComponentTransform, Delta);
	
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

	FVector PhysicsAngularVelocity = UpdatedPrimitive->GetPhysicsAngularVelocityInDegrees();
	
	const FTransform ComponentTransform = UpdatedPrimitive->GetComponentTransform();
	const FTransform InversedComponentTransform = ComponentTransform.Inverse();
	
	FVector LocalAngularVelocity = UKismetMathLibrary::TransformDirection(InversedComponentTransform, PhysicsAngularVelocity);

	const int RollSign = FMath::Sign(LocalAngularVelocity.X);
	LocalAngularVelocity.X += -RollSign
		* RotationData.RollDeceleration
		* DeltaTime;
	LocalAngularVelocity.X = RollSign == 1
		? FMath::Max(0.f, LocalAngularVelocity.X)
		: FMath::Min(0.f, LocalAngularVelocity.X);
	
	const int PitchSign = FMath::Sign(LocalAngularVelocity.Y);
	LocalAngularVelocity.Y += -PitchSign
		* RotationData.PitchDeceleration
		* DeltaTime;
	LocalAngularVelocity.Y = PitchSign == 1
		? FMath::Max(0.f, LocalAngularVelocity.Y)
		: FMath::Min(0.f, LocalAngularVelocity.Y);
	
	const int YawSign = FMath::Sign(LocalAngularVelocity.Z);
	LocalAngularVelocity.Z += -YawSign
		* RotationData.YawDeceleration
		* DeltaTime;
	LocalAngularVelocity.Z = YawSign == 1
		? FMath::Max(0.f, LocalAngularVelocity.Z)
		: FMath::Min(0.f, LocalAngularVelocity.Z);
	
	PhysicsAngularVelocity = UKismetMathLibrary::TransformDirection(ComponentTransform, LocalAngularVelocity);
	
	UpdatedPrimitive->SetPhysicsAngularVelocityInDegrees(PhysicsAngularVelocity);
}

void UHelicopterMovementComponent::ClampAngularVelocity()
{
	if(!UpdatedPrimitive)
		return;

	FVector PhysicsAngularVelocity = UpdatedPrimitive->GetPhysicsAngularVelocityInDegrees();
	
	const FTransform ComponentTransform = UpdatedPrimitive->GetComponentTransform();
	const FTransform InversedComponentTransform = ComponentTransform.Inverse();
	
	FVector LocalAngularVelocity = UKismetMathLibrary::TransformDirection(InversedComponentTransform, PhysicsAngularVelocity);

	const float HorizontalVelocity = USpeedConversionsLibrary::CmsToKmh(
		UpdatedPrimitive->GetPhysicsLinearVelocity().Size2D()
	);
	const float YawMaxSpeedScale = RotationData.YawMaxSpeedScaleFromVelocityCurve
		? RotationData.YawMaxSpeedScaleFromVelocityCurve->GetFloatValue(HorizontalVelocity)
		: 1.f;
	const float ScaledYawMaxSpeed = RotationData.YawMaxSpeed * YawMaxSpeedScale;
	
	LocalAngularVelocity.X = FMath::Clamp(
		LocalAngularVelocity.X,
		-RotationData.RollMaxSpeed,
		RotationData.RollMaxSpeed
	);
	LocalAngularVelocity.Y = FMath::Clamp(
		LocalAngularVelocity.Y,
		-RotationData.PitchMaxSpeed,
		RotationData.PitchMaxSpeed
	);
	LocalAngularVelocity.Z = FMath::Clamp(
		LocalAngularVelocity.Z,
		-ScaledYawMaxSpeed,
		ScaledYawMaxSpeed
	);
	
	PhysicsAngularVelocity = UKismetMathLibrary::TransformDirection(ComponentTransform, LocalAngularVelocity);
	
	UpdatedPrimitive->SetPhysicsAngularVelocityInDegrees(PhysicsAngularVelocity);
}

void UHelicopterMovementComponent::SyncPhysicsAndComponentMass()
{
	if(!UpdatedPrimitive)
		return;

	UpdatedPrimitive->SetMassOverrideInKg(NAME_None, GetActualMass(), true);
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