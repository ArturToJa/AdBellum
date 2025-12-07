// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/SimpleWeapon.h"
#include "Net/UnrealNetwork.h"
#include <Kismet/GameplayStatics.h>
#include <NiagaraFunctionLibrary.h>


// Sets default values
ASimpleWeapon::ASimpleWeapon()
{
	RootComponent = EBarrel;

	EBarrel = CreateDefaultSubobject<UEBBarrel>(TEXT("EBarrel"));

	bReplicates = true;
	bAlwaysRelevant = true;
	ReplicatedComponents.Add(EBarrel);
}

void ASimpleWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASimpleWeapon, EBarrel);
}

void ASimpleWeapon::Trigger_Implementation(bool trigger)
{
	Fire(trigger);
}

void ASimpleWeapon::Fire_Implementation(bool trigger)
{
	EBarrel->Shoot(trigger);
}

UTexture2D* ASimpleWeapon::GetWeaponHUDReticle_Implementation()
{
	return WeaponReticleTexture;
}

FVector ASimpleWeapon::GetCalibrationVector_Implementation(float Distance) 
{
	return EBarrel->GetForwardVector() * Distance * 100.0f + EBarrel->GetComponentLocation();
	//return EBarrel->GetComponentTransform().GetUnitAxis(EAxis::X)* Distance * 100.0f;
}
