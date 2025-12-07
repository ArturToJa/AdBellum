#pragma once
#include "System/GameModes/TeamDeathmatchGameMode.h"

TeamDeathmatchGameMode::TeamDeathmatchGameMode(TArray<uint8> Players) : GameModeBase()
{
	for (uint8 NumberOfPlayers : Players)
	{
		PlayersStateInGame.Emplace(NumberOfPlayers);
	}
}

TeamDeathmatchGameMode::~TeamDeathmatchGameMode()
{
	TeamWinsDelegate.Unbind();
	DrawDelegate.Unbind();
}

EGameMode TeamDeathmatchGameMode::GetGameModeType()
{
	return EGameMode::TeamDeathmatch;
}

void TeamDeathmatchGameMode::PlayerLost(int32 TeamIndex)
{
	PlayersStateInGame[TeamIndex]--;
	uint8 WinningIndex;
	uint8 AliveTeams = PlayersStateInGame.Num();
	for (int8 Index = 0; Index < PlayersStateInGame.Num(); ++Index)
	{
		if (PlayersStateInGame[Index] == 0)
		{
			AliveTeams--;
		}
		else
		{
			WinningIndex = Index;
		}
	}
	switch (AliveTeams)
	{
	case 0:
		// Draw
		DrawDelegate.ExecuteIfBound();
		break;
	case 1:
		// One winner
		TeamWinsDelegate.ExecuteIfBound(TeamIndex);
		break;
	default:
		// Game still in progress, do nothing
		break;
	}
}