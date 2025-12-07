// Designed by Hitman's Store, 2023

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PluginSettings.generated.h"

UCLASS(config = PluginSetting)
class RECOILANIMATION_API UPluginSettings : public UObject
{
	GENERATED_BODY()

public:
	UPluginSettings(const FObjectInitializer& Obj);

	/** Default location for curves */
	UPROPERTY(Config, EditAnywhere, Category = "Essentials")
	FString CurvesSavePath;

	/** Default location for Stored Data assets */
	UPROPERTY(Config, EditAnywhere, Category = "Essentials")
	FString DataSavePath;

	/** Should StoredData asset be created if null */
	UPROPERTY(Config, EditAnywhere, Category = "Essentials")
	bool bCreateDataAssetIfNull;
};