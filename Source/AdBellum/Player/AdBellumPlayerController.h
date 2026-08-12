// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/ALSPlayerController.h"
#include "IPlayer.h"
#include "OrderSystem/OrderType.h"
#include "Unit/BaseUnit.h"
#include "RTS_HUD.h"
#include <Character/ALSPlayerCameraManager.h>
#include "AdBellumPlayerController.generated.h"


class AHUD;
class ARTS_HUD;
class ARTSPlayer;
class AALSBaseCharacter;
class BaseOrder;
class ACharacterHUD;
class ABaseFormation;
/**
 * 
 */
UCLASS()
class ADBELLUM_API AAdBellumPlayerController : public AALSPlayerController, public IIPlayer
{
	GENERATED_BODY()

public:
	AAdBellumPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* NewPawn) override;
	virtual void AcknowledgePossession(APawn* NewPawn) override;
	UFUNCTION(Client, Reliable)
	void Client_AcknowledgePossession(APawn* NewPawn);

	UFUNCTION()
	void PossessAction(const FInputActionValue& Value);
	UFUNCTION()
	void CameraFreeRotateAction(const FInputActionValue& Value);
	UFUNCTION()
	void TriggerAction(const FInputActionValue& Value);
	UFUNCTION()
	void OrderStopAction(const FInputActionValue& Value);
	UFUNCTION()
	void OrderPatrolAction(const FInputActionValue& Value);
	UFUNCTION()
	void OrderMoveAction(const FInputActionValue& Value);
	UFUNCTION()
	void OrderHoldPositionAction(const FInputActionValue& Value);
	UFUNCTION()
	void OrderAttackAction(const FInputActionValue& Value);
	UFUNCTION()
	void PrimarySelectionAction(const FInputActionValue& Value);
	UFUNCTION()
	void SecondarySelectionAction(const FInputActionValue& Value);
	UFUNCTION()
	void ThirdSelectionAction(const FInputActionValue& Value);
	UFUNCTION(Server, Reliable)
	void Server_TriggerAction(FVector MouseLocation);
	UFUNCTION(BlueprintImplementableEvent)
	void EscapeAction(const FInputActionValue& Value);


	virtual int GetPlayerIndex_Implementation() override;
	virtual void SetPlayerIndex_Implementation(int index) override;
	virtual int GetTeamIndex_Implementation() override;
	virtual void SetTeamIndex_Implementation(int index) override;
	virtual void PossessCharacter_Implementation(APawn* Unit) override;
	virtual void UnpossessCharacter_Implementation() override;
	virtual void RemoveSelectedUnit_Implementation(APawn* Unit) override;
	virtual void SpawnRTSCamera_Implementation(FTransform SpawnTransform) override;
	virtual void NotifyActorReplicated_Implementation(AActor* ReplicatedActor) override;
	virtual void SetSelectionCircle_Implementation(AActor* SelectedActor, bool IsVisible) override;
	virtual void NotifyHUDRole_Implementation(EALSStationaryRole StationaryRole, AActor* ControlledActor) override;
	virtual AActor* GetPlayerStateActor_Implementation() override;
	virtual void GameEnded_Implementation(MatchResult Result) override;
	virtual void NotifyCharacterHUD_Implementation(FNotifyHUDData NotifyData) override;
	virtual void UpdateCameraDamageEffects_Implementation(float HPRatio) override;
	virtual void InitializeRTSHUD_Implementation(ABaseFormation* Formation) override;

	UFUNCTION(Client, Reliable)
	void Client_UpdateCameraDamageEffects(float HPRatio);

	UFUNCTION(Client, Reliable)
	void Client_GameEnded(MatchResult Result);

	UFUNCTION(BlueprintImplementableEvent)
	void BP_GameEnded(MatchResult Result);

	UFUNCTION(Client, Reliable)
	void Client_NotifyHUDRole(EALSStationaryRole StationaryRole, AActor* ControlledActor);

	UFUNCTION(Client, Reliable)
	void Client_SetPlayerCameraManager(FQuat NewRotation);

	UFUNCTION(Client, Reliable)
	void Client_SetSelectionCircle(AActor* SelectedActor, bool IsVisible);


	FTimerHandle RTSSpawnTimerHandle;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ARTSPlayer> RTSCameraClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ARTS_HUD> RTSHUDClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ACharacterHUD> CharacterHUDClass;

	void SpawnRTSHud();
	void SpawnCharacterHud();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(Server, Reliable)
	void SetSelectedFormations(const TArray<ABaseFormation*>& SelectedFormations);
	template<OrderEnum Type>
	void PerformOrder(AActor* TargetUnit, FVector TargetPosition);
	void RightMouseButtonPressed(bool Enabled);
	void SelectTargetForOrder(OrderEnum Type);

	UFUNCTION(Client, Reliable)
	void Client_SetUnitPrefab(const TArray<AActor*>& Units, const TArray<FMeshCreatorPrefabStruct>& Prefabs);

	UFUNCTION(Client, Reliable)
	void Client_OnWeaponCreated(const TArray<AActor*>& Weapons, const TArray<FUnitWeaponDataStruct>& WeaponPrefabs);

	UFUNCTION(Client, Reliable)
	void Client_OnHUDNotify(FNotifyHUDData NotifyData);

	UPROPERTY(ReplicatedUsing = OnRep_RTSPlayerSpawned)
	TObjectPtr<ARTSPlayer> RTSCameraPawn;
	UPROPERTY()
	TObjectPtr<ARTS_HUD> RTSHUD;
	UPROPERTY()
	TObjectPtr<ACharacterHUD> CharacterHUD;
	UPROPERTY(Replicated)
	int PlayerIndex;
	UPROPERTY(Replicated)
	int TeamIndex;

	UFUNCTION()
	void OnRep_RTSPlayerSpawned();

	UFUNCTION(Server, Reliable)
	void ServerPossessAction();
	UFUNCTION(Client, Reliable)
	void ClientClearSelection();

	// Networking
	UFUNCTION(Server, Reliable)
	void ServerNotifyActorReplicated(AActor* ReplicatedActor);

	TObjectPtr<AALSPlayerCameraManager> PlayerCameraManager;
private:
	bool IsEnemyUnit(AActor* TargetUnit);
	void PerformSmartOrder(AActor* TargetUnit);
	FVector GetMousePositionInWorld();
	AActor* GetActorFromMousePosition();
	void FindSuitableOrder();
	UFUNCTION(Server, Reliable)
	void Server_FindSuitableOrder(AActor* FoundActor, FVector MouseLocation);
	void MoveSuitableOrder(AActor* FoundActor, FVector MouseLocation);

	//TArray<APawn*> SelectedPawns;

	bool IsSelectingOrder = false;
	UPROPERTY(Replicated)
	OrderEnum SelectingOrder;
	UPROPERTY(Replicated)
	TObjectPtr<ABaseFormation> SelectionFormation;
};
