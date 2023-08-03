// Fill out your copyright notice in the Description page of Project Settings.


#include "CameraLookAroundComponent.h"

#include "Heli/LogHeli.h"
#include "Kismet/KismetMathLibrary.h"

UCameraLookAroundComponent::UCameraLookAroundComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bWantsInitializeComponent = true;
}

void UCameraLookAroundComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UCameraLookAroundComponent::InitializeComponent()
{
	Super::InitializeComponent();
	
	OwnerPawn = Cast<APawn>(GetOwner());
	if(!OwnerPawn)
	{
		HELI_ERR("Can't get pawn pointer due to cast failure");
	}
}

void UCameraLookAroundComponent::AddLookAround(float Yaw, float Pitch)
{
	if(!bIsLookAroundEnabled)
		return;
	
	CurrentYaw = UKismetMathLibrary::FClamp(CurrentYaw + Yaw, -YawLimit, YawLimit);
	CurrentPitch = UKismetMathLibrary::FClamp(CurrentPitch + Pitch, -PitchLimit, PitchLimit);
}

void UCameraLookAroundComponent::SetLookAngles(float Yaw, float Pitch)
{
	if(!bIsLookAroundEnabled)
		return;
	
	CurrentYaw = UKismetMathLibrary::FClamp(Yaw, -YawLimit, YawLimit);
	CurrentPitch = UKismetMathLibrary::FClamp(Pitch, -PitchLimit, PitchLimit);
}

bool UCameraLookAroundComponent::IsLookAroundEnabled() const
{
	return bIsLookAroundEnabled;
}

void UCameraLookAroundComponent::SetIsLookAroundEnabled(bool bEnable)
{
	bIsLookAroundEnabled = bEnable;
	
	if(!bEnable)
	{
		CurrentPitch = 0.f;
		CurrentYaw = 0.f;
	}
}

void UCameraLookAroundComponent::TickComponent(float DeltaTime, ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateControlRotation();
}

void UCameraLookAroundComponent::UpdateControlRotation()
{
	if(!OwnerPawn || !OwnerPawn->Controller)
		return;

	FRotator Rotator = OwnerPawn->GetActorRotation();

	if(bIsLookAroundEnabled)
	{
		FVector ActorUp = OwnerPawn->GetActorUpVector();
		FVector ActorRight = OwnerPawn->GetActorRightVector();
		
		FRotator YawRotator = UKismetMathLibrary::RotatorFromAxisAndAngle(ActorUp, CurrentYaw);
		FRotator PitchRotator = UKismetMathLibrary::RotatorFromAxisAndAngle(ActorRight, -CurrentPitch);
		
		FRotator RotatorToApply = UKismetMathLibrary::ComposeRotators(PitchRotator, YawRotator);
		
		Rotator = UKismetMathLibrary::ComposeRotators(Rotator, RotatorToApply);
	}

	OwnerPawn->Controller->SetControlRotation(Rotator);
}
