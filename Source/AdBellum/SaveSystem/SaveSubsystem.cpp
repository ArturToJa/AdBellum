// Fill out your copyright notice in the Description page of Project Settings.


#include "SaveSubsystem.h"
#include "Kismet/GameplayStatics.h"

void USaveSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	LoadOrCreateSquadSave();
	LoadOrCreateUnitSave();
	ValidateSquadPrefabData();
}

void USaveSystem::Deinitialize()
{

}

void USaveSystem::SaveSquadDataAsset(const FString& SquadName, const FUnitSaveData& PrefabDataArray, bool bOverwrite)
{
#if WITH_EDITOR
	if (DoesSquadPrefabExist(SquadName))
	{
		if (bOverwrite)
		{
			DefaultSquadsDataAsset->Modify();
			DefaultSquadsDataAsset->DefaultSquads[SquadName] = PrefabDataArray;
			DefaultSquadsDataAsset->MarkPackageDirty();
			UE_LOG(LogTemp, Warning, TEXT("Squad prefab overwritten in data asset: %s"), *SquadName);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Squad already exists in data asset: %s"), *SquadName);
			return;
		}
	}
	else
	{
		DefaultSquadsDataAsset->Modify();
		DefaultSquadsDataAsset->DefaultSquads.Add(SquadName, PrefabDataArray);
		DefaultSquadsDataAsset->MarkPackageDirty();
	}
#endif
}

void USaveSystem::SaveUnitDataAsset(const FString& UnitName, const FMeshCreatorPrefabStruct& PrefabData, bool bOverwrite)
{
#if WITH_EDITOR
	if (DoesUnitPrefabExist(UnitName))
	{
		if (bOverwrite)
		{
			DefaultSquadsDataAsset->Modify();
			DefaultSquadsDataAsset->DefaultUnits[UnitName] = PrefabData;
			DefaultSquadsDataAsset->MarkPackageDirty();
			UE_LOG(LogTemp, Warning, TEXT("Unit prefab overwritten in data asset: %s"), *UnitName);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Unit already exists in data asset: %s"), *UnitName);
			return;
		}
	}
	else
	{
		DefaultSquadsDataAsset->Modify();
		DefaultSquadsDataAsset->DefaultUnits.Add(UnitName, PrefabData);
		DefaultSquadsDataAsset->MarkPackageDirty();
	}
#endif
}

void USaveSystem::SaveSquads()
{
	if (SquadSaveGame)
	{
		FString SlotName = TEXT("SquadPrefabs");
		const int32 UserIndex = 0;
		UGameplayStatics::SaveGameToSlot(SquadSaveGame, SlotName, UserIndex);
		UE_LOG(LogTemp, Warning, TEXT("Squad prefabs saved in save file."));
	}
}

void USaveSystem::SaveSquadPrefab(const FString& SquadName, const FUnitSaveData& PrefabDataArray, bool bOverwrite)
{
	if (SquadSaveGame)
	{
		if (DoesSquadPrefabExist(SquadName))
		{
			if (bOverwrite)
			{
				SquadSaveGame->SquadPrefabs[SquadName] = PrefabDataArray;
				UE_LOG(LogTemp, Warning, TEXT("Squad prefab overwritten: %s"), *SquadName);
				return;
			}
			else
			{
				SquadSaveGame->SquadPrefabs.Add(SquadName + "_1", PrefabDataArray);
				UE_LOG(LogTemp, Warning, TEXT("Squad prefab saved: %s"), *(SquadName + "_1"));
			}
		}
		else
		{
			SquadSaveGame->SquadPrefabs.Add(SquadName, PrefabDataArray);
			UE_LOG(LogTemp, Warning, TEXT("Squad prefab saved: %s"), *SquadName);
		}
		SaveSquads();
	}
}

void USaveSystem::RemoveSquadPrefab(const FString& SquadName)
{
	if (SquadSaveGame)
	{
		if (SquadSaveGame->SquadPrefabs.Remove(SquadName) > 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("Squad prefab removed: %s"), *SquadName);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Squad prefab not found for removal: %s"), *SquadName);
		}
		SaveSquads();
	}
}

bool USaveSystem::DoesSquadPrefabExist(const FString& SquadName) const
{
	bool bExistsInSave = SquadSaveGame && SquadSaveGame->SquadPrefabs.Contains(SquadName);
	bool bExistsInDataAsset = DefaultSquadsDataAsset && DefaultSquadsDataAsset->DefaultSquads.Contains(SquadName);
	return bExistsInSave && bExistsInDataAsset;
}

TMap<FString, FUnitSaveData>& USaveSystem::GetSaveSquadPrefabs() const
{
	return SquadSaveGame->SquadPrefabs;
}

TMap<FString, FUnitSaveData>& USaveSystem::GetDefaultSquadPrefabs() const
{
	return DefaultSquadsDataAsset->DefaultSquads;
}

FUnitSaveData& USaveSystem::GetSquadPrefab(const FString& SquadName) const
{
	bool bExistsInSave = SquadSaveGame && SquadSaveGame->SquadPrefabs.Contains(SquadName);
	bool bExistsInDataAsset = DefaultSquadsDataAsset && DefaultSquadsDataAsset->DefaultSquads.Contains(SquadName);
	if (bExistsInSave)
	{
		return SquadSaveGame->SquadPrefabs[SquadName];
	}
	else if (bExistsInDataAsset)
	{
		return DefaultSquadsDataAsset->DefaultSquads[SquadName];
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Squad prefab not found: %s"), *SquadName);
		static FUnitSaveData EmptyData;
		return EmptyData;
	}
}

void USaveSystem::LoadOrCreateSquadSave()
{
	FString SlotName = TEXT("SquadPrefabs");
	const int32 UserIndex = 0;
	if (UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
	{
		SquadSaveGame = Cast<USquadSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
		if (SquadSaveGame)
		{
			UE_LOG(LogTemp, Warning, TEXT("Save loaded: SquadPrefabs"));
		}
	}
	else
	{
		SquadSaveGame = Cast<USquadSaveGame>(UGameplayStatics::CreateSaveGameObject(USquadSaveGame::StaticClass()));
		UGameplayStatics::SaveGameToSlot(SquadSaveGame, SlotName, UserIndex);
		UE_LOG(LogTemp, Warning, TEXT("Save created: SquadPrefabs"));
	}
}

void USaveSystem::SaveUnits()
{
	if (UnitSaveGame)
	{
		FString SlotName = TEXT("UnitPrefabs");
		const int32 UserIndex = 0;
		UGameplayStatics::SaveGameToSlot(UnitSaveGame, SlotName, UserIndex);
		UE_LOG(LogTemp, Warning, TEXT("Unit prefabs saved in save file."));
	}
}

void USaveSystem::SaveUnitPrefab(const FString& UnitName, const FMeshCreatorPrefabStruct& PrefabData, bool bOverwrite)
{
	if (UnitSaveGame)
	{
		if(DoesUnitPrefabExist(UnitName))
		{
			if (bOverwrite)
			{
				UnitSaveGame->UnitPrefabs[UnitName] = PrefabData;
				UE_LOG(LogTemp, Warning, TEXT("Unit prefab overwritten: %s"), *UnitName);
				return;
			}
			else
			{
				UnitSaveGame->UnitPrefabs.Add(UnitName + "_1", PrefabData);
				UE_LOG(LogTemp, Warning, TEXT("Unit prefab saved: %s"), *(UnitName + "_1"));
			}
		}
		else
		{
			UnitSaveGame->UnitPrefabs.Add(UnitName, PrefabData);
			UE_LOG(LogTemp, Warning, TEXT("Unit prefab saved: %s"), *UnitName);
		}
		SaveUnits();
	}
}

void USaveSystem::RemoveUnitPrefab(const FString& UnitName)
{
	if (UnitSaveGame)
	{
		if (UnitSaveGame->UnitPrefabs.Remove(UnitName) > 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("Unit prefab removed: %s"), *UnitName);
			ValidateSquadPrefabData();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Unit prefab not found for removal: %s"), *UnitName);
		}
		SaveUnits();
	}
}

bool USaveSystem::DoesUnitPrefabExist(const FString& UnitName) const
{
	bool bExistsInSave = UnitSaveGame && UnitSaveGame->UnitPrefabs.Contains(UnitName);
	bool bExistsInDataAsset = DefaultSquadsDataAsset && DefaultSquadsDataAsset->DefaultUnits.Contains(UnitName);
	return bExistsInSave && bExistsInDataAsset;
}

TMap<FString, FMeshCreatorPrefabStruct> USaveSystem::GetSaveUnitPrefabs() const
{
	return UnitSaveGame->UnitPrefabs;
}

TMap<FString, FMeshCreatorPrefabStruct> USaveSystem::GetDefaultUnitPrefabs() const
{
	return DefaultSquadsDataAsset->DefaultUnits;
}

FMeshCreatorPrefabStruct& USaveSystem::GetUnitPrefab(const FString& UnitName) const
{
	bool bExistsInSave = UnitSaveGame && UnitSaveGame->UnitPrefabs.Contains(UnitName);
	bool bExistsInDataAsset = DefaultSquadsDataAsset && DefaultSquadsDataAsset->DefaultUnits.Contains(UnitName);
	if (bExistsInSave)
	{
		return UnitSaveGame->UnitPrefabs[UnitName];
	}
	else if (bExistsInDataAsset)
	{
		return DefaultSquadsDataAsset->DefaultUnits[UnitName];
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Unit prefab not found: %s"), *UnitName);
		static FMeshCreatorPrefabStruct EmptyData;
		return EmptyData;
	}
}

void USaveSystem::LoadOrCreateUnitSave()
{
	FString SlotName = TEXT("UnitPrefabs");
	const int32 UserIndex = 0;

	if (UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
	{
		UnitSaveGame = Cast<UUnitSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
		if (UnitSaveGame)
		{
			UE_LOG(LogTemp, Warning, TEXT("Save loaded: UnitPrefabs"));
		}
	}
	else
	{
		UnitSaveGame = Cast<UUnitSaveGame>(UGameplayStatics::CreateSaveGameObject(UUnitSaveGame::StaticClass()));
		UGameplayStatics::SaveGameToSlot(UnitSaveGame, SlotName, UserIndex);
		UE_LOG(LogTemp, Warning, TEXT("Save created: UnitPrefabs"));
	}
}

void USaveSystem::ValidateSquadPrefabData() const
{
	for(TPair<FString, FUnitSaveData> SquadPrefab : SquadSaveGame->SquadPrefabs)
	{
		SquadPrefab.Value.UnitPrefabNames.RemoveAllSwap([this](const FString& UnitName) 
			{ 
				return !DoesUnitPrefabExist(UnitName); 
			});
	}
}