// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AI/ALSAIController.h"
#include "UnitAIController.generated.h"

/**
 * 
 */
UCLASS()
class ADBELLUM_API AUnitAIController : public AALSAIController
{
	GENERATED_BODY()


public:
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

	//virtual void UpdateControlRotation(float DeltaTime, bool bUpdatePawn) override;
private:
	float strengthSight;
	uint8 flagsSight;
};
