// Fill out your copyright notice in the Description page of Project Settings.


#include "PhysicsConvertionsLibrary.h"

float UPhysicsConvertionsLibrary::AccelMsAndMassToForce(float Acceleration, float Mass)
{
	return Acceleration * Mass;
}
