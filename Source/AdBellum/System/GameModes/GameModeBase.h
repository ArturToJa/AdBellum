#pragma once
#include "CoreMinimal.h"
#include "System/GameModes/GameModeTypes.h"

DECLARE_DELEGATE_OneParam(TeamWinsDelegate, int32);
DECLARE_DELEGATE(DrawDelegate);

class GameModeBase
{
public:
	GameModeBase() {};
	virtual EGameMode GetGameModeType() = 0;
	virtual ~GameModeBase() {};

	virtual void PlayerLost(int32 TeamIndex) = 0;

	TeamWinsDelegate TeamWinsDelegate;
	DrawDelegate DrawDelegate;
};