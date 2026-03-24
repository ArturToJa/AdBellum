// Fill out your copyright notice in the Description page of Project Settings.


#include "MainMenuGameState.h"
#include "System/AdBellumGameInstance.h"
#include "SaveSystem/SaveSubsystem.h"
#include "Net/UnrealNetwork.h"

AMainMenuGameState::AMainMenuGameState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	FPlayerDataStruct playerDataEntry;
	playerDataEntry.IsLocked = false;
	playerDataEntry.IsAI = false;
	playerDataEntry.IsEmpty = true;
	playerDataEntry.PlayerName = "Empty";
	TeamAlpha.Add(playerDataEntry);
	TeamAlpha.Add(playerDataEntry);
	TeamAlpha.Add(playerDataEntry);
	TeamAlpha.Add(playerDataEntry);
	TeamBeta.Add(playerDataEntry);
	TeamBeta.Add(playerDataEntry);
	TeamBeta.Add(playerDataEntry);
	TeamBeta.Add(playerDataEntry);
}

void AMainMenuGameState::BeginPlay()
{
	Super::BeginPlay();
	UAdBellumGameInstance* GameInstance = Cast<UAdBellumGameInstance>(GetGameInstance());
	check(GameInstance);
	TMap<FString, FPlayerSlotStruct> PlayerSlotMap = GameInstance->GetPlayerSlotMap();
	for (TPair<FString, FPlayerSlotStruct> Pair : PlayerSlotMap)
	{
		switch (Pair.Value.Type)
		{
		case PlayerType::Admin:
		case PlayerType::Player:
			if (Pair.Value.Team == 0)
			{
				FPlayerDataStruct& playerDataEntry = TeamAlpha[Pair.Value.Slot];
				playerDataEntry.IsAI = false;
				playerDataEntry.IsLocked = true;
			}
			else
			{
				FPlayerDataStruct& playerDataEntry = TeamBeta[Pair.Value.Slot];
				playerDataEntry.IsAI = false;
				playerDataEntry.IsLocked = true;
			}
			break;
		case PlayerType::EasyAI:
		case PlayerType::MediumAI:
		case PlayerType::HardAI:
			if (Pair.Value.Team == 0)
			{
				FPlayerDataStruct& playerDataEntry = TeamAlpha[Pair.Value.Slot];
				playerDataEntry.IsAI = true;
				playerDataEntry.IsLocked = false;
				playerDataEntry.PlayerName = UEnum::GetValueAsString(Pair.Value.Type);
			}
			else
			{
				FPlayerDataStruct& playerDataEntry = TeamBeta[Pair.Value.Slot];
				playerDataEntry.IsAI = true;
				playerDataEntry.IsLocked = false;
				playerDataEntry.PlayerName = UEnum::GetValueAsString(Pair.Value.Type);
			}
			break;
		case PlayerType::Empty:
			break;
		}
	}
}

void AMainMenuGameState::AddNewPlayer(FPlayerDataStruct PlayerInfo, int& team, int& slot, FString UniqueId)
{
	for (int i = 0; i < TeamAlpha.Num(); ++i)
	{
		FPlayerDataStruct& playerDataEntry = TeamAlpha[i];
		if (playerDataEntry.IsEmpty && !playerDataEntry.IsAI && !playerDataEntry.IsLocked)
		{
			playerDataEntry = PlayerInfo;

			FPlayerSlotStruct SlotStruct;
			SlotStruct.Slot = i;
			SlotStruct.Team = 0;
			SlotStruct.Type = PlayerType::Player;
			SlotStruct.MultiplayerData.IsAI = false;
			SlotStruct.MultiplayerData.PlayerName = PlayerInfo.PlayerName;

			UAdBellumGameInstance* GameInstance = Cast<UAdBellumGameInstance>(GetGameInstance());
			check(GameInstance);
			GameInstance->AddPlayerSlotData(UniqueId, SlotStruct);

			team = 0;
			slot = i;
			return;
		}
	}
	for (int i = 0; i < TeamBeta.Num(); ++i)
	{
		FPlayerDataStruct& playerDataEntry = TeamBeta[i];
		if (playerDataEntry.IsEmpty && !playerDataEntry.IsAI && !playerDataEntry.IsLocked)
		{
			playerDataEntry.IsEmpty = false;
			playerDataEntry.IsAI = PlayerInfo.IsAI;
			playerDataEntry.PlayerName = PlayerInfo.PlayerName;
			playerDataEntry.IsLocked = false;
			playerDataEntry.PlayerController = PlayerInfo.PlayerController;
			UpdateTeamData(1, i, playerDataEntry);

			FPlayerSlotStruct SlotStruct;
			SlotStruct.Slot = i;
			SlotStruct.Team = 1;
			SlotStruct.Type = PlayerType::Player;
			SlotStruct.MultiplayerData.IsAI = false;
			SlotStruct.MultiplayerData.PlayerName = PlayerInfo.PlayerName;

			UAdBellumGameInstance* GameInstance = Cast<UAdBellumGameInstance>(GetGameInstance());
			check(GameInstance);
			GameInstance->AddPlayerSlotData(UniqueId, SlotStruct);

			team = 1;
			slot = i;
			return;
		}
	}
}

void AMainMenuGameState::RemovePlayer(int team, int slot, FString UniqueId)
{
	if (team > 1 || team < 0) return;
	if (slot > 3 || slot < 0) return;
	switch (team)
	{
	case 0:
		TeamAlpha[slot].IsEmpty = true;
		TeamAlpha[slot].IsLocked = false;
		TeamAlpha[slot].IsAI = false;
		TeamAlpha[slot].PlayerName = "";
		TeamAlpha[slot].PlayerController = nullptr;
		UpdateTeamData(team, slot, TeamAlpha[slot]);
		break;
	case 1:
		TeamBeta[slot].IsEmpty = true;
		TeamBeta[slot].IsAI = false;
		TeamBeta[slot].IsLocked = false;
		TeamBeta[slot].PlayerName = "";
		TeamBeta[slot].PlayerController = nullptr;
		UpdateTeamData(team, slot, TeamBeta[slot]);
		break;
	}
	UAdBellumGameInstance* GameInstance = Cast<UAdBellumGameInstance>(GetGameInstance());
	check(GameInstance);
	GameInstance->RemovePlayerSlotData(UniqueId);
}

void AMainMenuGameState::OnPlayerLogout(AController* LeavingPlayer, FString UniqueId)
{
	for (int i = 0; i < TeamAlpha.Num(); ++i)
	{
		FPlayerDataStruct& playerDataEntry = TeamAlpha[i];
		if (playerDataEntry.PlayerController == LeavingPlayer)
		{
			playerDataEntry.IsEmpty = true;
			playerDataEntry.IsLocked = false;
			playerDataEntry.IsAI = false;
			playerDataEntry.PlayerName = "";
			playerDataEntry.PlayerController = nullptr;
			UpdateTeamData(0, i, TeamAlpha[i]);
			UAdBellumGameInstance* GameInstance = Cast<UAdBellumGameInstance>(GetGameInstance());
			check(GameInstance);
			GameInstance->RemovePlayerSlotData(UniqueId);
			return;
		}
	}
	for (int i = 0; i < TeamBeta.Num(); ++i)
	{
		FPlayerDataStruct& playerDataEntry = TeamBeta[i];
		if (playerDataEntry.PlayerController == LeavingPlayer)
		{
			playerDataEntry.IsEmpty = true;
			playerDataEntry.IsLocked = false;
			playerDataEntry.IsAI = false;
			playerDataEntry.PlayerName = "";
			playerDataEntry.PlayerController = nullptr;
			UpdateTeamData(1, i, TeamBeta[i]);
			UAdBellumGameInstance* GameInstance = Cast<UAdBellumGameInstance>(GetGameInstance());
			check(GameInstance);
			GameInstance->RemovePlayerSlotData(UniqueId);
			return;
		}
	}
}

void AMainMenuGameState::UpdatePlayer_Implementation(int team, int slot, FPlayerDataStruct PlayerInfo)
{
	if (team > 1 || team < 0) return;
	if (slot > 3 || slot < 0) return;
	switch (team)
	{
	case 0:
		TeamAlpha[slot] = PlayerInfo;
		break;
	case 1:
		TeamBeta[slot] = PlayerInfo;
		break;
	}
	UpdateTeamData(team, slot, PlayerInfo);
}

void AMainMenuGameState::RestorePlayer(FString PlayerNetId, FPlayerDataStruct PlayerData)
{
	UAdBellumGameInstance* GameInstance = Cast<UAdBellumGameInstance>(GetGameInstance());
	check(GameInstance);
	FPlayerSlotStruct PlayerSlotData = GameInstance->GetPlayerSlotData(PlayerNetId);
	UpdatePlayer(PlayerSlotData.Team, PlayerSlotData.Slot, PlayerData);
}

void AMainMenuGameState::NotifyNewPlayer(AController* NewPlayer)
{
	NotifyNewPlayerOnPlayerList(NewPlayer);
}

void AMainMenuGameState::SetMapIndex(int _MapIndex)
{
	NotifyNewMap(_MapIndex);
}

void AMainMenuGameState::SetGameModeIndex(int _GameModeIndex)
{
	NotifyNewGameMode(_GameModeIndex);
}

void AMainMenuGameState::SetTicketsIndex(int _TicketsIndex)
{
	NotifyNewTicketsIndex(_TicketsIndex);
}

void AMainMenuGameState::ServerSetPlayerSquad_Implementation(const FString& SquadName, const TArray<FMeshCreatorPrefabStruct>& UnitPrefabs, int Team, int Slot)
{
	switch (Team)
	{
	case 0:
		TeamAlpha[Slot].PlayerSquad.UnitPrefabDataArray = UnitPrefabs;
		break;
	case 1:
		TeamBeta[Slot].PlayerSquad.UnitPrefabDataArray = UnitPrefabs;
		break;
	}
	SetPlayerSquadName(SquadName, Team, Slot);
}

void AMainMenuGameState::SetPlayerSquad(const FString& SquadName, int Team, int Slot)
{
	USaveSystem* SaveSystem = GetGameInstance()->GetSubsystem<USaveSystem>();
	check(SaveSystem);
	TArray<FMeshCreatorPrefabStruct> SquadUnits;
	FUnitSaveData& UnitSaveData = SaveSystem->GetSquadPrefab(SquadName);
	for (const FString& UnitName : UnitSaveData.UnitPrefabNames)
	{
		SquadUnits.Add(SaveSystem->GetUnitPrefab(UnitName));
	}
	
	ServerSetPlayerSquad(SquadName, SquadUnits, Team, Slot);
}

void AMainMenuGameState::SetMultiplayerData(int GameModeIndex, int TicketsIndex)
{
	if (UAdBellumGameInstance* GameInstance = GetGameInstance<UAdBellumGameInstance>())
	{
		GameInstance->PopulateTeams(TeamAlpha, TeamBeta);
		GameInstance->SetGameMode(GameModeIndex);
		GameInstance->SetTickets(TicketsArray[TicketsIndex]);
		GameInstance->SetIsStartingNewMap(true);
	}
}