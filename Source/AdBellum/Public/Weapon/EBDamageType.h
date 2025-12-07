// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "Library/AdBellumEnumLibrary.h"
#include "EBDamageType.generated.h"


/**
 * 
 */
UCLASS()
class ADBELLUM_API UEBDamageType : public UDamageType
{
	GENERATED_BODY()


public:
	UEBDamageType();

	EBodyPart GetBodyPart();

	EBodyPart BodyPart;

	void SetBodyPart(EBodyPart BodyPart);
	
};

UCLASS()
class ADBELLUM_API UEBDamageTypeHEAD : public UEBDamageType 
{
	GENERATED_BODY()
public:
	UEBDamageTypeHEAD();
};
UCLASS()
class ADBELLUM_API UEBDamageTypeTORSO : public UEBDamageType
{
	GENERATED_BODY()
public:
	UEBDamageTypeTORSO();
};
UCLASS()
class ADBELLUM_API UEBDamageTypeLEGS : public UEBDamageType
{
	GENERATED_BODY()
public:
	UEBDamageTypeLEGS();
};
UCLASS()
class ADBELLUM_API UEBDamageTypeARMS : public UEBDamageType
{
	GENERATED_BODY()
public:
	UEBDamageTypeARMS();
};