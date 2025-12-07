#pragma once

#include "System/GameModes/GameModeBase.h"

class ADBELLUM_API TeamDeathmatchGameMode : public GameModeBase
{
public:
	TeamDeathmatchGameMode(TArray<uint8> Players);
	virtual ~TeamDeathmatchGameMode() override;
	virtual EGameMode GetGameModeType() override;

	virtual void PlayerLost(int32 TeamIndex) override;
private:
	TArray<uint8> PlayersStateInGame;
};