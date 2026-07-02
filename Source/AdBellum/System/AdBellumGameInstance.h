// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AdvancedFriendsGameInstance.h"
#include "Library/Weapon/WeaponPrefabStruct.h"
#include "Library/MeshCreatorParamsStruct.h"
#include "System/GameModes/GameModeTypes.h"
#include "SaveSystem/DefaultSquadsDataAsset.h"
#include "AdBellumGameInstance.generated.h"

USTRUCT(BlueprintType)
struct ADBELLUM_API FPlayerSquadDataStruct
{
public:
	GENERATED_USTRUCT_BODY();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TArray<FMeshCreatorPrefabStruct> UnitPrefabDataArray;
};

USTRUCT(BlueprintType)
struct ADBELLUM_API FPlayerDataStruct
{
	GENERATED_USTRUCT_BODY();

	FPlayerDataStruct()
	{
		PlayerName = "";
		IsAI = false;
		IsEmpty = true;
		IsLocked = false;
		PlayerController = nullptr;
	}

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FString PlayerName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool IsAI;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool IsEmpty;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool IsLocked;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	AController* PlayerController;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FPlayerSquadDataStruct PlayerSquad;

	bool operator==(const FPlayerDataStruct& Other)
	{
		return this->PlayerName == Other.PlayerName && IsAI == Other.IsAI && IsEmpty == Other.IsEmpty && PlayerController == Other.PlayerController;
	}
};

UENUM(BlueprintType)
enum class PlayerType : uint8
{
	Admin UMETA(DisplayName = "Admin"),
	Player UMETA(DisplayName = "Human"),
	EasyAI UMETA(DisplayName = "EasyAI"),
	MediumAI UMETA(DisplayName = "MediumAI"),
	HardAI UMETA(DisplayName = "HardAI"),
	Empty UMETA(DisplayName = "Empty")
};

struct ADBELLUM_API FMultiplayerDataStruct
{
	FString PlayerName;
	FString UniqueNetIdString;
	bool IsAI;
	FPlayerSquadDataStruct PlayerSquad;
};

USTRUCT(BlueprintType)
struct ADBELLUM_API FPlayerSlotStruct
{
	GENERATED_USTRUCT_BODY();

	FPlayerSlotStruct()
	{
		Type = PlayerType::Empty;
	}

	FMultiplayerDataStruct MultiplayerData;
	PlayerType Type;
	int Team;
	int Slot;
};

UCLASS()
class ADBELLUM_API UAdBellumGameInstance : public UAdvancedFriendsGameInstance
{
	GENERATED_BODY()

public:
	UAdBellumGameInstance(const FObjectInitializer& ObjectInitializer);

	void PopulateTeams(TArray<FPlayerDataStruct> TAlpha, TArray<FPlayerDataStruct> TBeta);
	TArray<TArray<FMultiplayerDataStruct>> GetTeams();
	void SetGameMode(int GameModeIndex);
	EGameMode GetGameMode();
	void SetTickets(int Tickets);
	int GetTickets();

	TMap<FString, FPlayerSlotStruct> GetPlayerSlotMap();
	FPlayerSlotStruct GetPlayerSlotData(FString PlayerNetId);
	void AddPlayerSlotData(FString PlayerNetId, FPlayerSlotStruct PlayerSlotData);
	void RemovePlayerSlotData(FString PlayerNetId);
	void ClearPlayerSlotData();
	void SetIsStartingNewMap(bool bIsStarting);

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	UDefaultSquadsDataAsset* DefaultSquadsDataAsset;

private:
	TArray<TArray<FMultiplayerDataStruct>> Teams;
	EGameMode GameMode = EGameMode::TeamDeathmatch;
	int StartingTickets = 5;
	bool bIsTravelling = false;


	TMap<FString, FPlayerSlotStruct> PlayerSlotMap;
};
