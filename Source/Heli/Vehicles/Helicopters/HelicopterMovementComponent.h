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
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float CurrentCollective { 0.f };

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float CollectiveIncreaseSpeed { 0.75f };
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float CollectiveDecreaseSpeed { 0.75f };
};

USTRUCT(BlueprintType)
struct FRotationData
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float PitchSpeed { 35.f };
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float RollSpeed { 30.f };

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float YawSpeed { 25.f };
	
};

USTRUCT(BlueprintType)
struct FPhysicsData
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float GravityZAcceleration { USpeedConversionsLibrary::MsToCms(-9.8f) };

	// Max speed in all directions, even facing straight down
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MaxSpeed { USpeedConversionsLibrary::KmhToCms(260.f) };

	// Mass of helicopter itself, without cargo
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MassKg { 7100.f };

	// Add mass here if you need to simulate some heavy cargo
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float AdditionalMassKg { 0.f };

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MaxLiftForce { UPhysicsConvertionsLibrary::AccelMsAndMassToForce(22.f, MassKg) };

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MinLiftForce { 0.f };

	// Values taken from US standard atmosphere air properties. Do not change them "just in case"
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float AirFrictionDensityAndDrag { 1.225f * 0.024f };

	// Approximate area that will face air
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float AirFrictionAreaMSqr { 20.f };

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<UCurveFloat> AirFrictionScaleFromVelocityCurve {};

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
	
};
