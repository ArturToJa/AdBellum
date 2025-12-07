// Designed by Hitman's Store, 2023

#include "PluginSettings.h"

UPluginSettings::UPluginSettings(const FObjectInitializer& Obj)
{
	CurvesSavePath = DataSavePath = FString("/Game/PRAS/");
	bCreateDataAssetIfNull = true;
}