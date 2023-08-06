// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/MovementComponent.h"
#include "Heli/BFLs/PhysicsConvertionsLibrary.h"
#include "Heli/BFLs/SpeedConversionsLibrary.h"
#include "HelicopterMovementComponent.generated.h"

USTRUCT(BlueprintType)
struct FCollectiveData
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(ClampMin=0.0, ClampMax=1.0))
	float CurrentCollective { 0.f };

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float CollectiveIncreaseSpeed { 0.45f };
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float CollectiveDecreaseSpeed { 0.45f };
};

USTRUCT(BlueprintType)
struct FRotationData
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float PitchSpeed { 35.f };
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float RollSpeed { 35.f };

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float YawSpeed { 35.f };
	
};

USTRUCT(BlueprintType)
struct FPhysicsData
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float GravityZAcceleration { USpeedConversionsLibrary::MsToCms(-9.8f) };

	// Max speed in all directions, even facing straight down
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MaxSpeed { USpeedConversionsLibrary::KmhToCms(320.f) };

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float AverageMaxSpeedScale { 0.8f };

	// Mass of helicopter itself, without cargo
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MassKg { 0.f };
	
	// Add mass here if you need to simulate some heavy cargo
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float AdditionalMassKg { 0.f };
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float LiftForceFromMaxCollective { 0.f };

	// It's better to start making it from two keys: (0; 0) (1;0)
	// then place new key at 0.45 and set it's scale so helicopter is going to start going up at this key
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<UCurveFloat> LiftForceScaleFromCollectiveCurve {};

	// It gets angle between world Up and component Up and passes it to the curve to find lift scale
	// we need it to not allow helicopter to fly on pitch = 60 using max collective
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<UCurveFloat> LiftScaleFromRotationCurve {};

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<UCurveFloat> HorizontalAirFrictionDecelerationToVelocityCurve {};

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<UCurveFloat> VerticalAirFrictionDecelerationToVelocityCurve {};
	
};

UCLASS(
	Blueprintable,
	HideCategories=(ComponentReplication, Replication, ComponentTick, PlanarMovement, MovementComponent, Activation),
	meta=(BlueprintSpawnableComponent)
)
class HELI_API UHelicopterMovementComponent : public UMovementComponent
{
	GENERATED_BODY()

public:
	UHelicopterMovementComponent();

	UFUNCTION(BlueprintCallable)
	void SetCollective(float NewCollocation);

	UFUNCTION(BlueprintCallable)
	void IncreaseCollective();

	UFUNCTION(BlueprintCallable)
	void DecreaseCollective();
	
	UFUNCTION(BlueprintCallable)
	void AddRotation(float PitchIntensity, float YawIntensity, float RollIntensity);
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual float GetGravityZ() const override;
	
	virtual float GetMaxSpeed() const override;

	float GetActualMass() const;
	
protected:
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	FPhysicsData PhysicsData {};
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	FCollectiveData CollectiveData {};

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	FRotationData RotationData {};
	
	virtual void BeginPlay() override;

private:
	
	float CalculateForceAmountBasedOnCollective() const;
	FVector CalculateCurrentCollectiveAccelerationVector() const;
	
	void UpdateVelocity(float DeltaTime);

	void ApplyGravityToVelocity(float DeltaTime);
	
	void ApplyAccelerationsToVelocity(float DeltaTime);

	void ApplyVelocityDamping(float DeltaTime);
	
	void ClampVelocityToMaxSpeed();

	void ApplyVelocityToLocation(float DeltaTime);

	float GetCurrentAngle() const;
	
};
