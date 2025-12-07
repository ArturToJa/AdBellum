// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/WeaponChildComponent.h"
#include <Weapon/SimpleWeapon.h>
#include <ComponentUtils.h>

UWeaponChildActorComponent::UWeaponChildActorComponent(const FObjectInitializer& Initializer) :UChildActorComponent(Initializer)
{
	SetIsReplicatedByDefault(true);
}

void UWeaponChildActorComponent::BeginPlay() 
{
	Super::BeginPlay();
	if (GetChildActor()) 
	{
		ControlledWeapon = Cast<ASimpleWeapon>(GetChildActor());
	}
}

void UWeaponChildActorComponent::Trigger(bool Value)
{
	//ControlledWeapon->SetTrigger(Value);
}

FVector UWeaponChildActorComponent::GetCalibrationVector(float Distance)
{
	return ControlledWeapon->GetCalibrationVector(Distance);
}