// Development console commands for changing a unit's weapons and sights at runtime.
// Target is the unit the player controls, or the selected unit while in the RTS camera.
//
//   adb.Weapon.List                              - weapon row names (weapon data table of the game mode)
//   adb.Sight.List                               - sight row names usable on the weapon in hand
//   adb.Weapon.Primary   <WeaponRow> [SightRow]  - replace the primary weapon
//   adb.Weapon.Secondary <WeaponRow> [SightRow]  - replace the secondary weapon
//   adb.Weapon.Special   <WeaponRow> [SightRow]  - replace the special weapon
//
// WeaponRow "None" only removes the weapon of that slot. Not available in shipping builds.

#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "Engine/DataTable.h"
#include "Player/AdBellumPlayerController.h"
#include "Unit/BaseUnit.h"
#include "Weapon/BaseWeapon.h"

#if !UE_BUILD_SHIPPING

namespace
{
	AAdBellumPlayerController* GetLocalController(UWorld* World)
	{
		return World ? Cast<AAdBellumPlayerController>(World->GetFirstPlayerController()) : nullptr;
	}

	void SetWeaponCommand(EWeaponSocketEnum Slot, const TCHAR* CommandName, const TArray<FString>& Args, UWorld* World)
	{
		AAdBellumPlayerController* Controller = GetLocalController(World);
		if (!Controller)
		{
			UE_LOG(LogTemp, Warning, TEXT("%s: no local player controller, start a game first"), CommandName);
			return;
		}
		if (Args.Num() < 1)
		{
			Controller->Client_DebugMessage(FString::Printf(TEXT("Usage: %s <WeaponRow or None> [SightRow]   (see adb.Weapon.List, adb.Sight.List)"), CommandName));
			return;
		}
		const FName WeaponRow(*Args[0]);
		const FName SightRow = Args.Num() > 1 ? FName(*Args[1]) : NAME_None;
		Controller->Server_DebugSetWeapon(Slot, WeaponRow, SightRow);
	}

	FAutoConsoleCommandWithWorldAndArgs GWeaponPrimaryCommand(
		TEXT("adb.Weapon.Primary"),
		TEXT("Replaces the primary weapon: adb.Weapon.Primary <WeaponRow or None> [SightRow]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			SetWeaponCommand(EWeaponSocketEnum::PRIMARY, TEXT("adb.Weapon.Primary"), Args, World);
		}));

	FAutoConsoleCommandWithWorldAndArgs GWeaponSecondaryCommand(
		TEXT("adb.Weapon.Secondary"),
		TEXT("Replaces the secondary weapon: adb.Weapon.Secondary <WeaponRow or None> [SightRow]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			SetWeaponCommand(EWeaponSocketEnum::SECONDARY, TEXT("adb.Weapon.Secondary"), Args, World);
		}));

	FAutoConsoleCommandWithWorldAndArgs GWeaponSpecialCommand(
		TEXT("adb.Weapon.Special"),
		TEXT("Replaces the special weapon: adb.Weapon.Special <WeaponRow or None> [SightRow]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			SetWeaponCommand(EWeaponSocketEnum::SPECIAL, TEXT("adb.Weapon.Special"), Args, World);
		}));

	FAutoConsoleCommandWithWorldAndArgs GWeaponListCommand(
		TEXT("adb.Weapon.List"),
		TEXT("Prints the weapon row names that can be given with adb.Weapon.Primary/Secondary/Special"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			// The weapon table belongs to the game mode, which only exists on the server
			if (AAdBellumPlayerController* Controller = GetLocalController(World))
			{
				Controller->Server_DebugListWeapons();
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GSightListCommand(
		TEXT("adb.Sight.List"),
		TEXT("Prints the sight row names usable on the weapon in hand"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			AAdBellumPlayerController* Controller = GetLocalController(World);
			if (!Controller)
			{
				return;
			}
			// Each weapon class names its own sight table, known on every machine
			const ABaseUnit* Unit = Controller->GetDebugTargetUnit();
			const ABaseWeapon* Weapon = Unit ? Cast<ABaseWeapon>(Unit->ActiveWeaponActor) : nullptr;
			const UDataTable* SightTable = Weapon ? Weapon->SightPrefabData.DataTable.Get() : nullptr;
			if (!SightTable)
			{
				Controller->Client_DebugMessage(TEXT("Sights: no weapon in hand, or its SightPrefabData has no data table"));
				return;
			}
			TArray<FString> Names;
			for (const FName& RowName : SightTable->GetRowNames())
			{
				Names.Add(RowName.ToString());
			}
			Controller->Client_DebugMessage(FString::Printf(TEXT("Sights for %s (%d): %s"), *Weapon->GetClass()->GetName(), Names.Num(), *FString::Join(Names, TEXT(", "))));
		}));
}

#endif
