// Designed by Hitman's Store, 2023

#include "StoredData.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "PluginSettings.h"
#include "RecoilAnimationComponent.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/MessageDialog.h"
#include "UObject/SavePackage.h"

UStoredData* UStoredData::FindDataAsset(const FString& SlotName, const UWorld* const World)
{
	/** If DataAsset is null, there're 2 possible scenarios:
	 * 1) Data asset just doesn't exist, in this case a new data asset will be created in PRAS folder
	 * 2) Data asset exists, but it hasn't been assigned yet
	 * p.s will be exeucted in editor only
	 */

	UE_LOG(LogRecoilAnimation, Warning, TEXT("%s data asset is NULL!"), *SlotName)

	if(!World->IsPlayInEditor())
	{
		/** not a valid state */
		return nullptr;
	}

	const auto PluginSettings = GetMutableDefault<UPluginSettings>();
	
	if(!PluginSettings)
	{
		UE_LOG(LogRecoilAnimation, Error, TEXT("Plugin Settings Are NULL"))
		return nullptr;
	}
	
	const FString PackagePath = PluginSettings->DataSavePath + SlotName;

	auto QuitSession = [&]() -> void
	{
		/** Quit game with error message */
		FMessageDialog::Open(EAppMsgType::Ok,
		                     FText::FromString(
			                     SlotName + TEXT(" data asset hasn't been assigned yet!")));
		World->GetFirstPlayerController()->ConsoleCommand("quit");
	};

	if(LoadPackage(nullptr, *PackagePath, LOAD_None))
	{
		QuitSession();
		return nullptr;
	}

	if(PluginSettings->bCreateDataAssetIfNull)
	{
		/** if loaded package is null, then new data asset will be created */
		UPackage* Package = CreatePackage(*PackagePath);
		UE_LOG(LogRecoilAnimation, Warning, TEXT("%s data asset doesn't exist! Creating new data asset..."), *SlotName)

		UStoredData* Loaded = NewObject<UStoredData>(Package, FName(*SlotName), RF_Public | RF_Standalone);
		Loaded->SaveData(Package, SlotName);

		return Loaded;
	}

	QuitSession();
	return nullptr;
}

void UStoredData::SaveData(UPackage* Package, const FString& SlotName)
{
	/** This creates or finds (if exists) package using the given path */
	const FString PackagePath = GetPackagePath(SlotName);
	
	if(!Package)
	{
		Package = CreatePackage(*PackagePath);
	}
	
	FAssetRegistryModule::AssetCreated(this);

	/** Getting the full path to the file */
	const FString FilePath = FPackageName::LongPackageNameToFilename(Package->GetPathName(),
																	 FPackageName::GetAssetPackageExtension());

	/** Finally saving just created package */
	FSavePackageArgs SavePackageArgs;

	SavePackageArgs.TopLevelFlags = EObjectFlags::RF_Public | EObjectFlags::RF_Standalone;
	SavePackageArgs.Error = GError;
	SavePackageArgs.bForceByteSwapping = true;
	SavePackageArgs.bWarnOfLongFilename = true;
	SavePackageArgs.SaveFlags = SAVE_NoError;
	
	const bool bSuccess = UPackage::SavePackage(Package, this, *FilePath, SavePackageArgs);
	
	Package->SetDirtyFlag(true);

	UE_LOG(LogRecoilAnimation, Warning, TEXT("Saved Package: %s, location: %s"),
	       bSuccess ? TEXT("SUCCESS") : TEXT("ERROR"), *FilePath)
}

FString UStoredData::GetPackagePath(const FString& SlotName) const
{
	/** Current data asset's location */
	auto String = GetPathName();
	if(String.RemoveFromEnd(FString(".") + SlotName))
	{
		return String;
	}

	const auto PluginSettings = GetMutableDefault<UPluginSettings>();
	
	if(!PluginSettings)
	{
		UE_LOG(LogRecoilAnimation, Error, TEXT("Plugin Settings Are NULL"))
		return FString("None");
	}
	
	return PluginSettings->DataSavePath + SlotName;
}