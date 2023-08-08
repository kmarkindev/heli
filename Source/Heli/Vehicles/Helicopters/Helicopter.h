// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Helicopter.generated.h"

class UHelicopterRootMeshComponent;
class UCameraLookAroundComponent;
class UCameraComponent;
class USpringArmComponent;
class UHelicopterMovementComponent;

UCLASS(Blueprintable, Abstract, HideCategories=(ComponentReplication, Replication, ActorTick))
class HELI_API AHelicopter : public APawn
{
	GENERATED_BODY()

public:

	inline static FName HelicopterMeshComponentName { TEXT("HelicopterMeshComponent") };
	inline static FName HelicopterMovementComponentName { TEXT("HelicopterMovementComponent") };
	inline static FName CameraLookAroundComponentName { TEXT("CameraLookAroundComponent") };
	inline static FName SpringArmComponentName { TEXT("SpringArmComponent") };
	inline static FName CameraComponentName { TEXT("CameraComponent") };
	
	AHelicopter();
	
	virtual void Tick(float DeltaTime) override;

	virtual void PostInitializeComponents() override;

protected:

	UPROPERTY()
	TObjectPtr<USpringArmComponent> CameraSpringArmComponent {};

	UPROPERTY()
	TObjectPtr<UCameraComponent> CameraComponent {};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UHelicopterRootMeshComponent> HelicopterMeshComponent {};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UHelicopterMovementComponent> HelicopterMovementComponent {};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UCameraLookAroundComponent> CameraLookAroundComponent {};
	
	virtual void BeginPlay() override;
	
private:

	void ConfigHelicopterMesh();

	void ConfigCameraAndSpringArm();
	
};
