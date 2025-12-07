#pragma once
#include "CoreMinimal.h"
#include "GameModeTypes.generated.h"

UENUM(BlueprintType)
enum class EGameMode : uint8
{
	TeamDeathmatch UMETA(DisplayName = "Team Deathmatch")
};

UENUM(BlueprintType)
enum class MatchResult : uint8
{
	Win UMETA(DisplayName = "Win"),
	Defeat UMETA(DisplayName = "Defeat"),
	Draw UMETA(DisplayName = "Draw")
};