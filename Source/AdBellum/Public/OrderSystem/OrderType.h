#pragma once

#include "CoreMinimal.h"
#include "OrderType.generated.h"

UENUM(BlueprintType)
enum class OrderEnum : uint8
{
	Attack UMETA(DisplayName = "Attack"),
	Move UMETA(DisplayName = "Move"),
	Stop UMETA(DisplayName = "Stop"),
	HoldPosition UMETA(DisplayName = "HoldPosition"),
	Patrol UMETA(DisplayName = "Patrol"),
	Follow UMETA(DisplayName = "Follow"),
	Smart UMETA(DisplayName = "Smart"),
	Interact UMETA(DisplayName = "Interact"),
	Enter UMETA(DisplayName = "Enter"),
	TakeCover UMETA(DisplayName = "TakeCover"),
	OccupyAOI UMETA(DisplayName = "OccupyAOI"),
	Reload UMETA(DisplayName = "Reload"),
	Training UMETA(DisplayName = "Training"),
};