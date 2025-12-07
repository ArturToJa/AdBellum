// Fill out your copyright notice in the Description page of Project Settings.


#include "Vehicle/GunnerSeatComponent.h"
#include "Character/ALSBaseCharacter.h"
#include "System/AdBellumGameState.h"
#include <Unit/BaseUnit.h>
#include <Net/UnrealNetwork.h>
#include <Weapon/WeaponChildComponent.h>



void UGunnerSeatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGunnerSeatComponent, WeaponArray);
}

bool UGunnerSeatComponent::SetOccupyingUnit(AALSBaseCharacter* InUnit)
{
	if (Super::SetOccupyingUnit(InUnit)) 
	{
		/*OccupyingUnit->GunnerWeaponChangedDelegate.BindUObject(this, &UGunnerSeatComponent::GetWeapon);*/
		SetWeaponArrayOwner(InUnit);
		AActor* ActtiveWeapon = GetWeapon(0);
		OccupyingUnit->ActiveWeaponActor = ActtiveWeapon;
		Multicast_BindDelegateToUnit(InUnit);
		return true;
	}
	else return false;
}

void UGunnerSeatComponent::SetWeaponArrayOwner(AALSBaseCharacter* InUnit) 
{
	UWeaponChildActorComponent* TempWeaponComp = nullptr;
	for (FComponentReference WeaponCompRef : WeaponArray) 
	{
		TempWeaponComp = Cast<UWeaponChildActorComponent>(WeaponCompRef.GetComponent(GetOwner()));
		if (TempWeaponComp && TempWeaponComp->GetChildActor())
		{
			if (InUnit)
			{
				TempWeaponComp->GetChildActor()->SetOwner(InUnit);
			}
			else 
			{
				TempWeaponComp->GetChildActor()->SetOwner(GetOwner());
			}
		}
	}
}

void UGunnerSeatComponent::Multicast_BindDelegateToUnit_Implementation(AALSBaseCharacter* InUnit)
{
	InUnit->GunnerWeaponChangedDelegate.BindUObject(this, &UGunnerSeatComponent::GetWeapon);
}

AActor* UGunnerSeatComponent::GetWeapon(int Id)
{
	if (WeaponArray.IsValidIndex(Id))
	{
		UWeaponChildActorComponent* SelectedWeaponComponent = nullptr;
		FComponentReference WeaponCompRef = WeaponArray[Id];
		SelectedWeaponComponent = Cast<UWeaponChildActorComponent>(WeaponCompRef.GetComponent(GetOwner()));
		if (SelectedWeaponComponent && SelectedWeaponComponent->GetChildActor())
		{
			return SelectedWeaponComponent->GetChildActor();
		}
		else return nullptr;
	}
	else return nullptr;

}
