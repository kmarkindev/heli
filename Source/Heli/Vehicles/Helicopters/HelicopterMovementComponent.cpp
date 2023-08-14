#include "HelicopterMovementComponent.h"

// Do not include editor-only dependencies for non editor builds
#if WITH_EDITOR
#include "EditorDialogLibrary.h"
#endif

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

void UHelicopterMovementComponent::CalculateForceNeededToStartGoingUp() const
{
	// Do not include code that depends on editor-only modules for game builds
#if WITH_EDITOR
	const float ActualMass = GetActualMass();

	const float GravityZ = UHeliConversionsLibrary::CmsToMs(GetGravityZ());
	const float ForceNeeded = UHeliConversionsLibrary::AccelMsAndMassToForce(-GravityZ + 1.f, ActualMass);
	const float Percent = ForceNeeded / PhysicsData.LiftForceFromMaxCollective * 100.f;

	const FString Result = FString::Printf(
	TEXT("For an actual mass %fkg, you need to apply a force %fN to start going up."
			" For specified LiftForceFromMaxCollective (%fN) it's %f%%."
			" Note: It doesn't take into account decelerations, only gravity."),
		ActualMass,
		ForceNeeded,
		PhysicsData.LiftForceFromMaxCollective,
		Percent);

	UEditorDialogLibrary::ShowMessage(
		FText::FromString(TEXT("Result")),
		FText::FromString(Result),
		EAppMsgType::Ok
	);
#endif
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
	
	const float Acceleration = UHeliConversionsLibrary::MsToCms(CurrentCollocationForceAmount / GetActualMass());
	
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

float UHelicopterMovementComponent::GetMaxAdditionalMass() const
{
	return PhysicsData.MaxAdditionalMassKg; 
}

float UHelicopterMovementComponent::GetCurrentCollective() const
{
	return CollectiveData.CurrentCollective;
}

float UHelicopterMovementComponent::GetCurrentAltitude() const
{
	if(!UpdatedPrimitive)
		return 0.f;

	UWorld* World = GetWorld();
	if(!World)
		return 0.f;
	
	FVector Start = UpdatedPrimitive->GetComponentLocation();
	FVector Direction = {0.f, 0.f, -1.f};
	float Distance = 1000.f * 100.f; // 1000 kilometers
	FVector End = Start + Direction * Distance;

	FHitResult HitResult {};
	bool bBlocked = World->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility);
	if(!bBlocked)
		return Distance;

	return FMath::Max((HitResult.ImpactPoint - Start).Length() + AltitudeOffset, 0.f);
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
	const float HorizontalSpeed = UHeliConversionsLibrary::CmsToKmh(PhysicsVelocity.Size2D());
	const float HorizontalAirFrictionDeceleration = PhysicsData.HorizontalAirFrictionDecelerationToVelocityCurve
		? UHeliConversionsLibrary::KmhToCms(
			PhysicsData.HorizontalAirFrictionDecelerationToVelocityCurve->GetFloatValue(HorizontalSpeed)
		)
		: 0.f;
	
	PhysicsVelocity += -PhysicsVelocity.GetSafeNormal2D() * HorizontalAirFrictionDeceleration * DeltaTime;

	// Apply vertical air friction
	// Note: Vertical Speed may be negative (in case of falling)
	const float VerticalSpeed = UHeliConversionsLibrary::CmsToKmh(PhysicsVelocity.Z);
	const float VerticalAirFrictionDeceleration = PhysicsData.VerticalAirFrictionDecelerationToVelocityCurve
		? UHeliConversionsLibrary::KmhToCms(
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

	const float HorizontalVelocity = UHeliConversionsLibrary::CmsToKmh(
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