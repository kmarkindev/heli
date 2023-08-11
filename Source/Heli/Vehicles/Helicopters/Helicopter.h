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

	void SetAdditionalMass(float NewMass, bool bAddToCurrent = false);
	float GetAdditionalMass() const;
	float GetRawMass() const;
	float GetActualMass() const;

	float GetCurrentCollective() const;

protected:

	UPROPERTY()
	TObjectPtr<USpringArmComponent> CameraSpringArmComponent {};

	UPROPERTY()
	TObjectPtr<UCameraComponent> CameraComponent {};

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UHelicopterRootMeshComponent> HelicopterMeshComponent {};
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UHelicopterMovementComponent> HelicopterMovementComponent {};

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UCameraLookAroundComponent> CameraLookAroundComponent {};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UPhysicalMaterial> HelicopterPhysicalMaterial {};
	
	virtual void BeginPlay() override;
	
private:

	void ConfigHelicopterMesh();

	void ConfigCameraAndSpringArm();
	
};
