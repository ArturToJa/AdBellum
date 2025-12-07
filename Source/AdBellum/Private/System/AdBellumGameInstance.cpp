// Fill out your copyright notice in the Description page of Project Settings.


#include "System/AdBellumGameInstance.h"

UAdBellumGameInstance::UAdBellumGameInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAdBellumGameInstance::PopulateTeams(TArray<FPlayerDataStruct> TAlpha, TArray<FPlayerDataStruct> TBeta)
{
	Teams.SetNum(2, true);
	for (FPlayerDataStruct& PlayerData : TAlpha)
	{
		if (!PlayerData.IsEmpty || PlayerData.IsAI)
		{
			FMultiplayerDataStruct MultiplayerData;
			MultiplayerData.IsAI = PlayerData.IsAI;
			MultiplayerData.PlayerName = PlayerData.PlayerName;
			MultiplayerData.PlayerSquad = PlayerData.PlayerSquad;
			if (APlayerState* PlayerState = (PlayerData.PlayerController != NULL) ? ToRawPtr(PlayerData.PlayerController->PlayerState) : NULL)
			{
				FUniqueNetIdPtr UserId = PlayerState->GetUniqueId().GetUniqueNetId();
				if (UserId.IsValid())
				{
					MultiplayerData.UniqueNetIdString = UserId->ToString();
				}
			}
			Teams[0].Add(MultiplayerData);
		}
	}

	for (FPlayerDataStruct& PlayerData : TBeta)
	{
		if (!PlayerData.IsEmpty || PlayerData.IsAI)
		{
			FMultiplayerDataStruct MultiplayerData;
			MultiplayerData.IsAI = PlayerData.IsAI;
			MultiplayerData.PlayerName = PlayerData.PlayerName;
			MultiplayerData.PlayerSquad = PlayerData.PlayerSquad;
			if (APlayerState* PlayerState = (PlayerData.PlayerController != NULL) ? ToRawPtr(PlayerData.PlayerController->PlayerState) : NULL)
			{
				FUniqueNetIdPtr UserId = PlayerState->GetUniqueId().GetUniqueNetId();
				if (UserId.IsValid())
				{
					MultiplayerData.UniqueNetIdString = UserId->ToString();
				}
			}
			Teams[1].Add(MultiplayerData);
		}
	}
}

TArray<TArray<FMultiplayerDataStruct>> UAdBellumGameInstance::GetTeams()
{
	return Teams;
}

void UAdBellumGameInstance::SetGameMode(int GameModeIndex)
{
	GameMode = (EGameMode)GameModeIndex;
}

EGameMode UAdBellumGameInstance::GetGameMode()
{
	return GameMode;
}

void UAdBellumGameInstance::SetTickets(int Tickets)
{
	StartingTickets = Tickets;
}

int UAdBellumGameInstance::GetTickets()
{
	return StartingTickets;
}

TMap<FString, FPlayerSlotStruct> UAdBellumGameInstance::GetPlayerSlotMap()
{
	return PlayerSlotMap;
}

FPlayerSlotStruct UAdBellumGameInstance::GetPlayerSlotData(FString PlayerNetId)
{
	if (FPlayerSlotStruct* PlayerSlotData = PlayerSlotMap.Find(PlayerNetId))
	{
		return *PlayerSlotData;
	}
	return FPlayerSlotStruct();
}

void UAdBellumGameInstance::AddPlayerSlotData(FString PlayerNetId, FPlayerSlotStruct PlayerSlotData)
{
	if (!bIsTravelling)
	{
		PlayerSlotMap.Add(PlayerNetId, PlayerSlotData);
	}
}

void UAdBellumGameInstance::RemovePlayerSlotData(FString PlayerNetId)
{
	if (!bIsTravelling)
	{
		PlayerSlotMap.Remove(PlayerNetId);
	}
}

void UAdBellumGameInstance::ClearPlayerSlotData()
{
	if (!bIsTravelling)
	{
		PlayerSlotMap.Empty();
	}
}

void UAdBellumGameInstance::SetIsStartingNewMap(bool bIsStarting)
{
	bIsTravelling = bIsStarting;
}