// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/Selectable.h"
#include "Interfaces/ITargetable.h"
#include "TargetManual.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class USceneComponent;
class ATargetField;

// Practice shooting-range target, placed directly in the level (see the Placement section below) - C++
// replacement for BP_TargetManual/BP_TargetField. Spawns its own scored hit-zones (ATargetField) on the server
// only and relies on ordinary actor replication (plus RepScore/RepDamage below) to give every client - not just
// the host - a correctly networked target and live score/damage feedback.
//
// Placement: 10 placement markers (FieldMarkers, named "1".."10" to match the score each is worth - see
// BeginPlay) must be positioned/scaled per-instance in the editor, same as the old Blueprint's manually-scaled
// target fields. They are hidden at runtime; only the ATargetField actors spawned from them are visible/hittable.
UCLASS()
class ADBELLUM_API ATargetManual : public AActor, public ISelectable, public IITargetable
{
	GENERATED_BODY()

public:
	ATargetManual();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> DefaultSceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MainTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Middle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTextRenderComponent> ScoreText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTextRenderComponent> DamageText;

	// Placement markers for the 10 scored fields, named "1".."10" - that name is parsed as each spawned
	// ATargetField's point value (see BeginPlay), exactly like the original Blueprint. Scale/position each in
	// the editor per placed instance; they are hidden at runtime.
	// Transient: this array must always come straight from the constructor loop, never from a saved property
	// value - a stale serialized (null) array from an earlier compile was otherwise silently overriding it on
	// load, leaving every marker null and BeginPlay spawning nothing.
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TArray<TObjectPtr<UStaticMeshComponent>> FieldMarkers;

	// Server-only: the ATargetField actors spawned from FieldMarkers.
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Target")
	TArray<TObjectPtr<ATargetField>> SpawnedFields;

	// Current score/damage, replicated so every client's ScoreText/DamageText stays correct, not just the
	// server's. Called by a field (server-side only - see ATargetField::OnAnyDamageReceived) whenever it takes
	// damage.
	void NotifyScore(int32 Points, float Damage);

protected:
	virtual void BeginPlay() override;

	// IITargetable - BP_TargetManual overrode these with the same values; everything else it overrode (team
	// index aside) matched the interface's own generated defaults already, so only these two need a C++
	// override here.
	virtual bool IsAlive_Implementation() override;
	virtual int32 GetTeamIndex_Implementation() override;

	// Used by Unit AI as the aim point for this target - Middle's world location (attached to MainTarget's
	// TargetCenter socket, see constructor), not a fixed offset like the other IITargetable location getters.
	virtual FVector GetChestLocation_Implementation() override;

private:
	UPROPERTY(ReplicatedUsing = OnRep_RepScore)
	int32 RepScore = 0;

	UPROPERTY(ReplicatedUsing = OnRep_RepDamage)
	float RepDamage = 0.0f;

	UFUNCTION()
	void OnRep_RepScore();

	UFUNCTION()
	void OnRep_RepDamage();

	void UpdateScoreDisplay();
	void UpdateDamageDisplay();
	void ClearDamageText();

	FTimerHandle DamageClearTimerHandle;
};
