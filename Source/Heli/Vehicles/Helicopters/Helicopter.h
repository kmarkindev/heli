// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Helicopter.generated.h"

class UCameraLookAroundComponent;
class UCameraComponent;
class USpringArmComponent;
class UHelicopterMovementComponent;

UCLASS(Blueprintable)
class HELI_API AHelicopter : public APawn
{
	GENERATED_BODY()

public:

	inline static FName HelicopterMeshComponentName { TEXT("HelicopterMeshComponent") };
	inline static FName HelicopterMovementComponentName { TEXT("HelicopterMovementComponent") };
	inline static FName CameraLookAroundComponentName { TEXT("CameraLookAroundComponent") };
	
	AHelicopter();
	
	virtual void Tick(float DeltaTime) override;

	virtual void PostInitializeComponents() override;

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<USkeletalMeshComponent> HelicopterMesh {};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UHelicopterMovementComponent> HelicopterMovementComponent {};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UCameraLookAroundComponent> CameraLookAroundComponent {};

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<USpringArmComponent> CameraSpringArmComponent {};

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UCameraComponent> CameraComponent {};
	
	virtual void BeginPlay() override;

	
	
private:

	void ConfigHelicopterMesh();

	void ConfigSpringArmAndCamera();
	
};
