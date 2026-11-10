// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TargetField.generated.h"

class UStaticMeshComponent;
class ATargetManual;

// One scored hit-zone of a shooting-range target. Spawned server-only by ATargetManual::BeginPlay from one of
// its placement markers (see ATargetManual) and replicated down to every client as a real networked actor -
// unlike the old Blueprint version, where every machine (including clients) independently spawned its own
// disconnected, non-networked copy from the same markers.
UCLASS()
class ADBELLUM_API ATargetField : public AActor
{
	GENERATED_BODY()

public:
	ATargetField();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// The target this field reports hits to. Replicated mainly so it is never null on a client inspecting this
	// actor - scoring itself is entirely server-side (see OnAnyDamageReceived).
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Target")
	TObjectPtr<ATargetManual> OwningTarget;

	// Score value of this field, taken from its placement marker's name (see ATargetManual::BeginPlay).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target")
	int32 Points = 0;

protected:
	virtual void BeginPlay() override;

private:
	// Bound only on the server (see BeginPlay) - forwards every hit to OwningTarget so its replicated
	// score/damage display updates for every client.
	UFUNCTION()
	void OnAnyDamageReceived(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);
};
