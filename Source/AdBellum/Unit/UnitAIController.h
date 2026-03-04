// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AI/ALSAIController.h"
#include "OrderSystem/Orderable.h"
#include "Weapon/IWeapon.h"
#include "UnitAIController.generated.h"

class UOrdersManager;

UCLASS()
class ADBELLUM_API AUnitAIController : public AALSAIController, public IOrderable
{
	GENERATED_BODY()


public:
	AUnitAIController();
	UFUNCTION(BlueprintImplementableEvent)
	void Activate();
	UFUNCTION(BlueprintImplementableEvent)
	void Deactivate();
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void ClearTarget();
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SetTarget(AActor* TargetUnit); //setting focus and notify aim for unit
	UFUNCTION(BlueprintImplementableEvent)
	void OnDamageReceived(float Damage, AActor* DamagingActor);
	UFUNCTION(BlueprintCallable)
	void DecodeStrengthAndFlags(float EncodedValue);

	float GetSightStrength() const { return strengthSight; }
	uint8 GetSightFlags() const { return flagsSight; }

	//Orderable
	virtual class UOrdersManager* GetOrdersManagerComponent_Implementation() override;
	virtual void Stop_Implementation(FVector TargetPosition) override;
	virtual void MoveOrder_Implementation(FVector TargetPosition) override;
	virtual void AttackTarget_Implementation(UObject* TargetObject) override;
	virtual void AttackLocation_Implementation(FVector TargetPosition) override;
	virtual void DoCrouch_Implementation() override;
	virtual void DoCrawl_Implementation() override;
	virtual void DoStandUp_Implementation() override;

	

	//virtual void UpdateControlRotation(float DeltaTime, bool bUpdatePawn) override;
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UOrdersManager> OrdersManagerComponent;

private:
	float strengthSight;
	uint8 flagsSight;
	FTimerHandle AIAttackTimer;
	void StopTriggerTimer();
	void WeaponTriggerAction(float TriggerDuration = 0.1f);
	// Compute burst/trigger duration (seconds) from weapon and target using weapon/unit combat data
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI|Combat")
	float ComputeTriggerDuration(AActor* WeaponActor, UObject* TargetObject) const;
};
