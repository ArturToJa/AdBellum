// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/ALSCharacter.h"
#include "Formation/Formable.h"
#include "ArmedUnitInterface.h"
#include "Interfaces/ITargetable.h"
#include "Interfaces/Customizable.h"
#include "Interfaces/Selectable.h"
#include "Weapon/EBDamageType.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Engine/NetSerialization.h"
#include "Library/AdBellumEnumLibrary.h"
#include "Library/MeshCreatorParamsStruct.h"
#include "Util/MeshMergeFunctionLibrary.h"
#include "Library/Weapon/WeaponSocketEnum.h"
#include "Library/Weapon/WeaponPrefabStruct.h"
#include "RecoilAnimationComponent.h"
#include "UnitAIController.h"
#include "Perception/AISightTargetInterface.h"
#include "OptimizationComponent.h"
#include "BaseUnit.generated.h"



/**
 * 
 */

class UOrdersManager;
class URecoilComponent;
class ABaseWeapon;
class UNetworkComponent;
class ABaseFormation;

UENUM(BlueprintType)
enum class ETargetBodyPart : uint8
{
	HEAD,
	CHEST,
	LEFT_ARM,
	RIGHT_ARM,
	LEFT_LEG,
	RIGHT_LEG
};

UCLASS()
class ADBELLUM_API ABaseUnit : public AALSCharacter, public IFormable, public ISelectable, public IArmedUnitInterface, public IITargetable, public ICustomizable, public IAISightTargetInterface
{
	GENERATED_BODY()

public:
	ABaseUnit(const FObjectInitializer& ObjectInitializer);
	
	//ALSInput
	virtual void AddRecoil_Implementation() override;

	virtual void StopRecoil_Implementation() override;

	UFUNCTION(Server, Reliable)
	void Server_PlayRecoil(bool Active);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayRecoil(bool Active);

	//UFUNCTION(Client, Reliable)
	//void Client_PlayRecoil(bool Active);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<URecoilAnimationComponent> RecoilAnimationComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNetworkComponent> NetworkComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UOptimizationProxyComponent* OptimizationComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UAIPerceptionStimuliSourceComponent* AIPerception;

	UFUNCTION(BlueprintCallable, Category = "Customization")
	void SetVisibleMeshCustom(USkeletalMesh* NewSkeletalMesh);

	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Customization")
	void Server_SetVisibleMeshCustom(USkeletalMesh* NewSkeletalMesh);

	void SetupBodyMesh(const FMeshCreatorPrefabStruct& PrefabData);

	void SetupHeadMesh(const FMeshCreatorPrefabStruct& PrefabData);

	void HandlePrefabDefinition(TArray<FMeshCreatorDataInputStruct> MeshDefinitions, TArray<TSoftObjectPtr<USkeletalMesh>> AdditionalMeshes);

	//usage of BP Mesh Creator

	//WEAPON HANDLING
	//UFUNCTION(BlueprintCallable, Category = "Weapon")
	//void SetupWeapons();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
	TArray<ABaseWeapon*> WeaponArray;

	UFUNCTION(BlueprintCallable,NetMulticast, Reliable)
	void Multicast_WeaponArrayUpdate(ABaseWeapon* Weapon, EWeaponSocketEnum SocketEnum);

	UFUNCTION(BlueprintCallable)
	void SocketWeapon(ABaseWeapon* Weapon, EWeaponSocketEnum SocketEnum);

	FName PrimaryWeaponSocketName = "mainWeaponSocket";
	FName SecondaryWeaponSocketName = "secondaryWeaponSocket";
	FName SpecialWeaponSocketName = "specialWeaponSocket";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
	EWeaponSocketEnum CurrentWeaponSocket;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
	EWeaponSocketEnum TargetWeaponSocket = EWeaponSocketEnum::NONE;

	//UFUNCTION(BlueprintCallable, Category = "Weapons")
	//void PrepareLoadout();

	//UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Weapons")
	//void Server_PrepareLoadout();

	//END WEAPON HANDLING

	//DAMAGE 
	UFUNCTION()
	void OnAnyDamageReceived(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Damage")
	TMap<EBodyPart, float> ArmourMap;


	//ITARGETABLE GET ARMOUR PARAM BY EBODYPART
	virtual float GetArmourParamValueForBodyPart_Implementation(EBodyPart BodyPart) override;

	
	void NotifyDeath();

	//disable aiming on player char, disable hip fire, disable weapon trigger
	void HandleWeaponStatesOnPlayerDepossessed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Damage")
	void BP_NotifyDeath();

	UFUNCTION(BlueprintImplementableEvent, Category = "Damage")
	void BP_RespawnUnit();


	UFUNCTION(Server, Reliable, WithValidation)
	void ServerUpdateHealth(float Value);
	bool ServerUpdateHealth_Validate(float Value);
	void ServerUpdateHealth_Implementation(float Value);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastNotifyRagdoll(bool bRagdoll);

	// === HP Regeneration System ===
	FTimerHandle RegenerationTimerHandle;
	FTimerHandle RegenerationTickHandle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Regeneration")
	float RegenDelay = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Regeneration")
	float MAJOR_REGEN_TIME = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Regeneration")
	float MEDIUM_REGEN_TIME = 7.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Regeneration")
	float MINOR_REGEN_TIME = 10.0f;

	UFUNCTION()
	void StartHPRegen();

	UFUNCTION()
	void HandleHPRegen();

	UFUNCTION()
	void StopHPRegen();

	UFUNCTION()
	void ResetHPRegenTimer();


	////INPUT INTERFACE

	void HandleTriggerAction(bool Value);

	UFUNCTION(Server, Reliable)
	void Server_AimAction(bool bValue);

	UFUNCTION(Server, Reliable)
	void Server_AimActionCompleted();

	void HandlePressedADS();

	void HandleReleasedADS();

	void HandleNonStationaryAimAction(bool Value);

	void HandleStationaryAimAction(bool Value);

	FTimerHandle CheckAimCollisionTimer;

	void CheckAimCollision();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|SX")
	USoundBase* SoundADSPressed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|SX")
	USoundBase* SoundADSReleased;

	UFUNCTION(Server, Reliable)
	void Server_InteractionAction();

	virtual void AimAction_Implementation(bool bValue) override;

	virtual void AimActionCompleted_Implementation() override;

	virtual void InteractionAction_Implementation() override;

	virtual void TriggerAction_Implementation(bool Value) override;

	virtual void TriggerActionCompleted_Implementation() override;

	virtual void SprintAction_Implementation(bool bValue) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Input")
	bool bIsReloading = false;

	//Check AALSPlayerController::PrimarySelectionAction
	//UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
	virtual void PrimarySelectionAction_Implementation() override;

	//UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
	virtual void SecondarySelectionAction_Implementation() override;

	//UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
	virtual void ThirdSelectionAction_Implementation() override;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Weapon")
	void HandleWeaponSwitch(EWeaponSocketEnum DesiredWeaponSocket);

	//0 - primary
	//1 - seconadry
	//2 - special
	void HandleStationaryWeaponSwitch(int WeaponId);


	//Selectable 
	virtual void SetSelectionCircle_Implementation(bool Visible) override;
	virtual bool IsAlive_Implementation();
	virtual void SetOwningPlayer_Implementation(AActor* Player) override;
	virtual AActor* GetOwningPlayer_Implementation() override;
	virtual void SetTeamIndex_Implementation(int32 Index) override;
	virtual int32 GetTeamIndex_Implementation() override;
	virtual void SetInstanceIndex_Implementation(int32 Index) override;
	virtual int32 GetInstanceIndex_Implementation() override;
	virtual FVector GetSelectionCircleScale_Implementation() override;
	virtual FVector GetSelectionCircleLocation_Implementation() override;
	virtual int GetUnitType_Implementation() override;

	//ArmedUnitInterface
	virtual AActor* GetWeapon_Implementation() override;
	virtual void OnWeaponUpdated_Implementation(AActor* Weapon) override;
	virtual bool IsReloading_Implementation() override;
	virtual void GetUnitCombatDataStruct_Implementation(FUnitCombatDataStruct& CombatData) override;

	//ITargetable
	virtual FVector GetHeadLocation_Implementation() override;
	virtual FVector GetChestLocation_Implementation() override;
	virtual FVector GetLeftArmLocation_Implementation() override;
	virtual FVector GetRightArmLocation_Implementation() override;
	virtual FVector GetLeftLegLocation_Implementation() override;
	virtual FVector GetRightLegLocation_Implementation() override;
	virtual FVector GetWeaponLocation_Implementation() override;
	virtual TArray<AActor*> IsTargetedBy_Implementation() override;
	virtual void SetIsTargetedBy_Implementation(AActor* Actor, bool IsTargeted) override;
	virtual void SetIsSeenBy_Implementation(ABaseFormation* Actor, bool IsSeen) override;
	virtual void ClearTarget_Implementation() override;

	//Customizable
	virtual void ConfigureUnit_Implementation(const FMeshCreatorPrefabStruct& UnitPrefab) override;
	virtual void ConfigureWeapon_Implementation(ABaseWeapon* Weapon, EWeaponSocketEnum SocketEnum) override;

	//Formable
	virtual void SetFormation_Implementation(ABaseFormation* Formation) override;
	virtual ABaseFormation* GetFormation_Implementation() override;
	virtual void RespawnUnit_Implementation(FTransform RespawnTransform) override;

	//AISightTargetInterface
	virtual UAISense_Sight::EVisibilityResult CanBeSeenFrom(const FCanBeSeenFromContext& Context,
		FVector& OutSeenLocation, int32& OutNumberOfLoSChecksPerformed, int32& OutNumberOfAsyncLosCheckRequested,
		float& OutSightStrength, int32* UserData = nullptr, const FOnPendingVisibilityQueryProcessedDelegate* Delegate = nullptr) override;
	
	//montages
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	UAnimMontage* RecoilMontage;

	void PossessByAIController();

protected:

	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	// Helpers for possession transitions
	void HandleServerPlayerPossessed();
	void HandleClientPlayerPossessed();
	void HandleServerAIPossessed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Possession")
	void BP_OnPlayerPossessed(bool value);


	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	AUnitAIController* BackupAIController = nullptr;

	UPROPERTY(Replicated)
	int32 TeamIndex;
	UPROPERTY(Replicated)
	AActor* PlayerPtr;
	int32 SelectionCircleIndex;

	// prevent re-triggering while already firing
	bool bTriggerActive = false;

	FUnitCombatDataStruct UnitCombatData;

	// weapon trigger handling
	void WeaponTriggerAction();

	TArray<AActor*> TargetingAtActorArray;
	TArray<ABaseFormation*> SeenByFormation;

	FTimerHandle UnitDiedTimer;
	void HideActorOnDeath();

	UFUNCTION(Server, Reliable)
	void NotifyClearTargets();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Customization")
	FSkeletalMeshMergeParams MeshMergeParams;

	UPROPERTY(Replicated)
	ABaseFormation* OwningFormation;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Aim")
	bool HipFire = false;

	UFUNCTION(BlueprintCallable, Category = "ALS|Aim")
	bool GetHipFire();

	UFUNCTION(BlueprintCallable, Category = "ALS|Aim")
	void SetHipFire(bool Value);

	bool TriggerActive = false;

	bool WasPlayerControlled = false;

	// aim setup throttle to avoid redundant expensive calls
	UPROPERTY()
	TWeakObjectPtr<UObject> LastAimTarget;

	// time in seconds when last aim setup was invoked
	float LastAimSetupTime = 0.0f;

	// minimum interval between aim setup calls from AttackTarget (seconds)
	inline static constexpr float AimSetupCooldown = 0.2f;
};

