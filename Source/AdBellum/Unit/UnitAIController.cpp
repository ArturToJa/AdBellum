// Fill out your copyright notice in the Description page of Project Settings.


#include "UnitAIController.h"
#include "ArmedUnitInterface.h"
#include "OrderSystem/OrdersManager.h"
#include "Library/UnitCombatData.h"
#include "Library/WeaponCombatData.h"
#include "Weapon/IWeapon.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

AUnitAIController::AUnitAIController()
{
	OrdersManagerComponent = CreateDefaultSubobject<UOrdersManager>(TEXT("Order Manager Component"));
}

void AUnitAIController::DecodeStrengthAndFlags(float EncodedValue)
{
	uint32 Packed;
	FMemory::Memcpy(&Packed, &EncodedValue, sizeof(float));

	// Extract
	flagsSight = Packed & 0b00111111;
	uint32 StrengthQuantized = (Packed >> 6) & 0x3FF; // 10 bitów

	// Dequantize
	strengthSight = (float)StrengthQuantized / 1023.0f;

	bool bFlag0 = (flagsSight & (1 << 0)) != 0;
	bool bFlag1 = (flagsSight & (1 << 1)) != 0;
	bool bFlag2 = (flagsSight & (1 << 2)) != 0;
	bool bFlag3 = (flagsSight & (1 << 3)) != 0;
	bool bFlag4 = (flagsSight & (1 << 4)) != 0;
	bool bFlag5 = (flagsSight & (1 << 5)) != 0;
	FString string = "";
	string += bFlag0 ? "1" : "0";
	string += bFlag1 ? "1" : "0";
	string += bFlag2 ? "1" : "0";
	string += bFlag3 ? "1" : "0";
	string += bFlag4 ? "1" : "0";
	string += bFlag5 ? "1" : "0";

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
		FString::Printf(TEXT("Decoded Sight Strength: %f, Flags: %s"), strengthSight, *string));
}

//ORDERABLE INTERFACE
class UOrdersManager* AUnitAIController::GetOrdersManagerComponent_Implementation()
{
	return OrdersManagerComponent;
}

void AUnitAIController::Stop_Implementation(FVector TargetPosition)
{
	StopMovement();
}

void AUnitAIController::MoveOrder_Implementation(FVector TargetPosition)
{
	MoveToLocation(TargetPosition, 7.5f, true, true, true);
}
void AUnitAIController::AttackTarget_Implementation(UObject* TargetObject) {

	AActor* ActiveWeaponActor = IArmedUnitInterface::Execute_GetWeapon(GetPawn());
	IIWeapon::Execute_SetupAim(ActiveWeaponActor, TargetObject);

	float TriggerDuration = ComputeTriggerDuration(ActiveWeaponActor, TargetObject);

	UKismetSystemLibrary::PrintString(GetWorld(), FString::Printf(TEXT("TriggerDuration = %f"), TriggerDuration), true, true);
	WeaponTriggerAction(TriggerDuration);
}

float AUnitAIController::ComputeTriggerDuration(AActor* WeaponActor, UObject* TargetObject) const
{
	if (!WeaponActor || !GetPawn())
	{
		return 0.01f;
	}

	FWeaponCombatDataStruct WeaponData;
	IIWeapon::Execute_GetWeaponCombatData(WeaponActor, WeaponData);

	FUnitCombatDataStruct UnitData;
	IArmedUnitInterface::Execute_GetUnitCombatDataStruct(GetPawn(), UnitData);

	float Distance = 0.0f;
	AActor* TargetActor = Cast<AActor>(TargetObject);
	if (TargetActor)
	{
		Distance = FVector::Dist(GetPawn()->GetActorLocation(), TargetActor->GetActorLocation())/100;
	}

	float PerceivedDistance = Distance * UnitData.DistanceMultiplier;

	float BaseDuration = WeaponData.BurstDurationSlope * PerceivedDistance + WeaponData.BurstDurationIntercept;
	if (BaseDuration < 0.0f)
	{
		return WeaponData.MinBurstDuration;
	}

	float ClampedDuration = FMath::Clamp(BaseDuration, WeaponData.MinBurstDuration, WeaponData.MaxBurstDuration);

	float TriggerDuration = ClampedDuration * UnitData.BurstDurationMultiplier;

	if (TriggerDuration <= 0.0f)
	{
		TriggerDuration = WeaponData.MinBurstDuration;
	}
	return TriggerDuration;
}

void AUnitAIController::WeaponTriggerAction(float TriggerDuration)
{
	AActor* ActiveWeaponActor = IArmedUnitInterface::Execute_GetWeapon(GetPawn());
	if (ActiveWeaponActor)
	{
		IIWeapon::Execute_Trigger(ActiveWeaponActor, true);
		if (GetWorld())
		{
			GetWorldTimerManager().ClearTimer(AIAttackTimer);
			GetWorldTimerManager().SetTimer(AIAttackTimer, this, &AUnitAIController::StopTriggerTimer, TriggerDuration, false);
		}
	}
}

void AUnitAIController::StopTriggerTimer()
{
	AActor* ActiveWeaponActor = IArmedUnitInterface::Execute_GetWeapon(GetPawn());
	if (!ActiveWeaponActor) return;

	IIWeapon::Execute_Trigger(ActiveWeaponActor, false);
}

void AUnitAIController::AttackLocation_Implementation(FVector TargetPosition)
{
	AActor* ActiveWeaponActor = IArmedUnitInterface::Execute_GetWeapon(GetPawn());
	bool bTriggerActive = IIWeapon::Execute_IsTriggerActive(ActiveWeaponActor);
	if (!ActiveWeaponActor) return;
	if (!bTriggerActive)
	{
		IIWeapon::Execute_Trigger(ActiveWeaponActor, true);
		if (GetWorld())
		{
			GetWorldTimerManager().ClearTimer(AIAttackTimer);
			GetWorldTimerManager().SetTimer(AIAttackTimer, this, &AUnitAIController::StopTriggerTimer, FMath::FRandRange(0.2f, 0.75f), false);
		}
	}
}

void AUnitAIController::DoCrouch_Implementation()
{
	//if (!bIsCrouched)
	//{
	//	Crouch();
	//}
}


void AUnitAIController::DoCrawl_Implementation()
{

}
void AUnitAIController::DoStandUp_Implementation()
{
	//if (bIsCrouched)
	//{
	//	UnCrouch();
	//}
}

//END ORDERABLE INTERFACE
