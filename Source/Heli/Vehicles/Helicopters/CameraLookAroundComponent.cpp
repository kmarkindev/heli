// Fill out your copyright notice in the Description page of Project Settings.


#include "CameraLookAroundComponent.h"

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
		UE_LOG(LogTemp, Error, TEXT("%s: can't get pawn pointer due to cast failure"), UE_SOURCE_LOCATION);
	}
}

void UCameraLookAroundComponent::AddLookAround(float X, float Y)
{
	if(!bIsLookAroundEnabled)
		return;
	
	CurrentYaw = UKismetMathLibrary::FClamp(CurrentYaw + X, -YawLimit, YawLimit);
	CurrentPitch = UKismetMathLibrary::FClamp(CurrentPitch + Y, -PitchLimit, PitchLimit);
}

void UCameraLookAroundComponent::SetLookAngles(float Pitch, float Yaw)
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

	UpdateControlRotation(DeltaTime);
}

void UCameraLookAroundComponent::UpdateControlRotation(float DeltaTime)
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
