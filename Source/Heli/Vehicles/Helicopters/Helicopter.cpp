// Fill out your copyright notice in the Description page of Project Settings.


#include "Helicopter.h"

AHelicopter::AHelicopter()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AHelicopter::BeginPlay()
{
	Super::BeginPlay();
}

void AHelicopter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AHelicopter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

