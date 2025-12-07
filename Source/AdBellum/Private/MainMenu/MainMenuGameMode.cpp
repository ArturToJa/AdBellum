// Fill out your copyright notice in the Description page of Project Settings.


#include "MainMenu/MainMenuGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MainMenu/MainMenuGameState.h"

void AMainMenuGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	bInLobby = UGameplayStatics::HasOption(Options, TEXT("GameMode"));
	if (bInLobby)
	{
		GameModeIndex = FCString::Atoi(*UGameplayStatics::ParseOption(Options, TEXT("GameMode")));
	}
}

void AMainMenuGameMode::StartGame()
{
	if (AMainMenuGameState* MMGameState = GetGameState<AMainMenuGameState>())
	{
		MMGameState->SetMultiplayerData(GameModeIndex, TicketsIndex);
		FString Command = "servertravel " + MMGameState->MapArray[MapIndex].MapAsset.GetAssetName();
		UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), Command, nullptr);
	}
	
}

void AMainMenuGameMode::SetMap(int _MapIndex)
{
	this->MapIndex = _MapIndex;
	if (AMainMenuGameState* MMGameState = GetGameState<AMainMenuGameState>())
	{
		MMGameState->SetMapIndex(_MapIndex);
	}
}

void AMainMenuGameMode::SetGameMode(int _GameModeIndex)
{
	this->GameModeIndex = _GameModeIndex;
	if (AMainMenuGameState* MMGameState = GetGameState<AMainMenuGameState>())
	{
		MMGameState->SetGameModeIndex(_GameModeIndex);
	}
}

void AMainMenuGameMode::SetTickets(int _TicketsIndex)
{
	this->TicketsIndex = _TicketsIndex;
	if (AMainMenuGameState* MMGameState = GetGameState<AMainMenuGameState>())
	{
		MMGameState->SetTicketsIndex(_TicketsIndex);
	}
}

void AMainMenuGameMode::OnPostLogin(AController* NewPlayer)
{
	if (bInLobby)
	{
		if (APlayerState* PlayerState = NewPlayer->GetPlayerState<APlayerState>())
		{
			FPlayerDataStruct playerDataEntry;
			playerDataEntry.IsAI = false;
			playerDataEntry.PlayerName = PlayerState->GetPlayerName();
			playerDataEntry.IsEmpty = false;
			playerDataEntry.PlayerController = NewPlayer;
			if (AMainMenuGameState* MMGameState = GetGameState<AMainMenuGameState>())
			{
				//MMGameState->SetMapIndex(MapIndex);
				//MMGameState->SetGameModeIndex(GameModeIndex);
				
				//MMGameState->SetTicketsIndex(TicketsIndex);
				int team;
				int slot;
				MMGameState->AddNewPlayer(playerDataEntry, team, slot, PlayerState->GetUniqueId().GetUniqueNetId()->ToString());
				int numberOfPlayers = MMGameState->PlayerArray.Num();
				if (numberOfPlayers == 1)
				{
					MMGameState->NotifyRole(NewPlayer, LobbyRole::Admin);
				}
				else
				{
					MMGameState->NotifyRole(NewPlayer, LobbyRole::Player);
				}
				MMGameState->NotifyOwnership(NewPlayer, team, slot);
				MMGameState->UpdateTeamData(team, slot, playerDataEntry);
				MMGameState->NotifyNewPlayer(NewPlayer);
			}
		}
	}
}

void AMainMenuGameMode::HandleSeamlessTravelPlayer(AController*& C)
{
	Super::HandleSeamlessTravelPlayer(C);
	if (bInLobby)
	{
		if (APlayerState* PlayerState = C->GetPlayerState<APlayerState>())
		{
			FUniqueNetIdPtr UserId = PlayerState->GetUniqueId().GetUniqueNetId();
			if (UserId.IsValid())
			{
				FString UserIdString = UserId->ToString();

				FPlayerDataStruct playerDataEntry;
				playerDataEntry.IsAI = false;
				playerDataEntry.PlayerName = PlayerState->GetPlayerName();
				playerDataEntry.IsEmpty = false;
				playerDataEntry.IsLocked = false;
				playerDataEntry.PlayerController = C;

				if (AMainMenuGameState* MMGameState = GetGameState<AMainMenuGameState>())
				{
					MMGameState->NotifyNewPlayer(C);
					MMGameState->RestorePlayer(UserIdString, playerDataEntry);
				}
			}
		}
	}
}

void AMainMenuGameMode::Logout(AController* ExitingPlayer)
{
	if (AMainMenuGameState* MMGameState = GetGameState<AMainMenuGameState>())
	{
		MMGameState->OnPlayerLogout(ExitingPlayer, ExitingPlayer->GetPlayerState<APlayerState>()->GetUniqueId().GetUniqueNetId()->ToString());
	}
}