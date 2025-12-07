// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"


UENUM(BlueprintType)
enum class EWeaponSocketEnum : uint8
{
	PRIMARY = 0,
	SECONDARY = 1,
	SPECIAL = 2,
	NONE = 255
};