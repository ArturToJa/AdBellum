// Copyright:       Copyright (C) 2022 Doğa Can Yanıkoğlu
// Source Code:     https://github.com/dyanikoglu/ALS-Community


#pragma once

#include "CoreMinimal.h"
#include "Character/ALSBaseCharacter.h"
#include "Library/ALSMobilityDataInterface.h"
#include "ALSCharacter.generated.h"

/**
 * Specialized character class, with additional features like held object etc.
 */
UCLASS(Blueprintable, BlueprintType)
class ALSV4_CPP_API AALSCharacter : public AALSBaseCharacter, public IALSMobilityDataInterface
{
	GENERATED_BODY()

public:
	AALSCharacter(const FObjectInitializer& ObjectInitializer);

	/** Implemented on BP to update held objects */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "ALS|HeldObject")
	void UpdateHeldObject();

	UFUNCTION(BlueprintCallable, Category = "ALS|HeldObject")
	void ClearHeldObject();

	UFUNCTION(BlueprintCallable, Category = "ALS|HeldObject")
	void AttachToHand(UStaticMesh* NewStaticMesh, USkeletalMesh* NewSkeletalMesh,
	                  class UClass* NewAnimClass, bool bLeftHand, FVector Offset);

	virtual void RagdollStart() override;

	virtual void RagdollEnd() override;

	virtual ECollisionChannel GetThirdPersonTraceParams(FVector& TraceOrigin, float& TraceRadius) override;

	virtual FTransform GetThirdPersonPivotTarget() override;

	virtual FVector GetFirstPersonCameraTarget() override;

	virtual FVector GetFirstPersonCameraTargetADS() override;

	virtual FRotator GetFirstPersonCameraRotationADS() override;

	virtual EALSGait GetGait_Implementation() override;

	virtual EALSStance GetStance_Implementation() override;

protected:
	virtual void Tick(float DeltaTime) override;

	virtual void BeginPlay() override;

	virtual void OnOverlayStateChanged(EALSOverlayState PreviousState) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Implement on BP to update animation states of held objects */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "ALS|HeldObject")
	void UpdateHeldObjectAnimations();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Component")
	TObjectPtr<USceneComponent> HeldObjectRoot = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Component")
	TObjectPtr<USkeletalMeshComponent> SkeletalMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Component")
	TObjectPtr<UStaticMeshComponent> StaticMesh = nullptr;

	//UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "ALS|Component")
	//AActor* ActiveWeaponActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Aim")
	bool UsingADS = false;

	UFUNCTION(BlueprintCallable, Category = "ALS|Aim")
	bool GetUsingADS();

	UFUNCTION(BlueprintCallable, Category = "ALS|Aim")
	void SetUsingADS(bool Value);

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadWrite, Category = "Damage")
	float HP = 5000.0f;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadWrite, Category = "Damage")
	float MaxHP = 5000.0f;

	float GetHP();

	float GetMaxHP();

private:
	bool bNeedsColorReset = false;
};
