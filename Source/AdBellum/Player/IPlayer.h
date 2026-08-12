// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "System/GameModes/GameModeTypes.h"
#include "IPlayer.generated.h"

class ABaseUnit;

UENUM(BlueprintType)
enum class ENotifyHudType : uint8
{
	WeaponShot
};

USTRUCT(BlueprintType)
struct FNotifyHUDData
{
	GENERATED_USTRUCT_BODY();
	ENotifyHudType NotifyType;
};

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UIPlayer : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ADBELLUM_API IIPlayer
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IPlayer")
		int GetPlayerIndex();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IPlayer")
		void SetPlayerIndex(int index);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IPlayer")
		int GetTeamIndex();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IPlayer")
		void SetTeamIndex(int index);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IPlayer")
		void PossessCharacter(APawn* Unit);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IPlayer")
		void UnpossessCharacter();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IPlayer")
		void RemoveSelectedUnit(APawn* Unit);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IPlayer")
		void SpawnRTSCamera(FTransform SpawnTransform);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IPlayer")
		void NotifyActorReplicated(AActor* ReplicatedActor);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IPlayer")
		void SetSelectionCircle(AActor* SelectedActor, bool IsVisible);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IPlayer")
		void NotifyHUDRole(EALSStationaryRole StationaryRole, AActor* ControlledActor);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IPlayer")
		AActor* GetPlayerStateActor();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IPlayer")
		void GameEnded(MatchResult Result);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IPlayer")
		void NotifyCharacterHUD(FNotifyHUDData NotifyData);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IPlayer")
		void UpdateCameraDamageEffects(float HPRatio);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IPlayer")
		void InitializeRTSHUD(ABaseFormation* Formation);
};
