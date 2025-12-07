// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/StationaryWeapon.h"
#include <Net/UnrealNetwork.h>

AStationaryWeapon::AStationaryWeapon() 
{
	WeaponMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMeshComponent"));
	RootComponent = WeaponMeshComponent;

	EBarrel->SetupAttachment(WeaponMeshComponent);
}

void AStationaryWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AStationaryWeapon, WeaponMeshComponent);
}

//add camera inputs and decide when to delegate to vehicle or to weapon actor