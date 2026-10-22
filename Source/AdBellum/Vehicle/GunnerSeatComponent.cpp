// Fill out your copyright notice in the Description page of Project Settings.


#include "GunnerSeatComponent.h"
#include "Character/ALSBaseCharacter.h"
#include "System/AdBellumGameState.h"
#include <Unit/BaseUnit.h>
#include <Net/UnrealNetwork.h>
#include <Weapon/WeaponChildComponent.h>



void UGunnerSeatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGunnerSeatComponent, WeaponArray);
	DOREPLIFETIME(UGunnerSeatComponent, ReplicatedAim);
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

//AIM
FVector2D UGunnerSeatComponent::ClampAim(FVector2D Aim) const
{
	Aim.X = bLimitYaw ? FMath::Clamp(Aim.X, MinYaw, MaxYaw) : FRotator::NormalizeAxis(Aim.X);
	Aim.Y = FMath::Clamp(Aim.Y, MinPitch, MaxPitch);
	return Aim;
}

bool UGunnerSeatComponent::UsesLocalAim() const
{
	return OccupyingUnit && LocalAimUnit == OccupyingUnit && !GetOwner()->HasAuthority();
}

FVector2D UGunnerSeatComponent::GetAim() const
{
	return UsesLocalAim() ? LocalAim : ReplicatedAim;
}

void UGunnerSeatComponent::AddAimInput(float YawInput, float PitchInput)
{
	if (!OccupyingUnit)
	{
		return;
	}

	if (GetOwner()->HasAuthority())
	{
		ReplicatedAim = ClampAim(ReplicatedAim + FVector2D(YawInput * YawRate, -PitchInput * PitchRate));
		return;
	}

	if (LocalAimUnit != OccupyingUnit)
	{
		LocalAimUnit = OccupyingUnit;
		LocalAim = ReplicatedAim;
	}
	LocalAim = ClampAim(LocalAim + FVector2D(YawInput * YawRate, -PitchInput * PitchRate));
}

void UGunnerSeatComponent::SetAimFromClient(FVector2D NewAim)
{
	NewAim = ClampAim(NewAim);

	const float Now = GetWorld()->GetTimeSeconds();
	if (MaxAimSpeed > 0.0f)
	{
		// Never allow less than one frame at 10 fps worth of turn, so a late packet does not freeze the turret
		const float MaxStep = MaxAimSpeed * FMath::Clamp(Now - LastClientAimTime, 0.0f, 0.1f) + MaxAimSpeed * 0.1f;
		const float YawStep = bLimitYaw ? NewAim.X - ReplicatedAim.X : FMath::FindDeltaAngleDegrees(ReplicatedAim.X, NewAim.X);
		NewAim.X = ReplicatedAim.X + FMath::Clamp(YawStep, -MaxStep, MaxStep);
		NewAim.Y = ReplicatedAim.Y + FMath::Clamp(NewAim.Y - ReplicatedAim.Y, -MaxStep, MaxStep);
		NewAim = ClampAim(NewAim);
	}
	LastClientAimTime = Now;
	ReplicatedAim = NewAim;
}

void UGunnerSeatComponent::SetAim(float Yaw, float Pitch)
{
	if (GetOwner()->HasAuthority())
	{
		ReplicatedAim = ClampAim(FVector2D(Yaw, Pitch));
	}
}
