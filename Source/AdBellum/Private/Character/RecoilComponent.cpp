// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/RecoilComponent.h"
#include <Unit/BaseUnit.h>
#include <Kismet/KismetSystemLibrary.h>


URecoilComponent::URecoilComponent(const FObjectInitializer& ObjectInitializer)
	: UActorComponent(ObjectInitializer)
{
}

URecoilComponent::URecoilComponent(const FObjectInitializer& ObjectInitializer, APlayerController* InOwnerController, float InitVerticalMultiplier, float InitHorizontalMultiplier)
	: UActorComponent(ObjectInitializer)
{
	VerticalRecoilMultiplier = InitVerticalMultiplier;
	HorizontalRecoilMultiplier = InitHorizontalMultiplier;
	OwnerController = InOwnerController;
}


void URecoilComponent::AddRecoil(float VerticalValue, float HorizontalValue, TSubclassOf<class UCameraShakeBase> ShakeClass)
{
	if (!OwnerController)
	{
		ABaseUnit* CurrentUnit = Cast<ABaseUnit>(GetOwner());
		OwnerController = Cast<APlayerController>(CurrentUnit->GetController());
	}

	if (OwnerController){
	//IncreaseRecoilMultiplier();
	OwnerController->AddPitchInput(-1 * VerticalValue * VerticalRecoilMultiplier);
	if (FMath::RandBool())
		{
			OwnerController->AddYawInput(HorizontalValue * HorizontalRecoilMultiplier);
		}
		else
		{
			OwnerController->AddYawInput(-1 * HorizontalValue * HorizontalRecoilMultiplier);
		}

	PlayCameraShake(ShakeClass, 1.0f);
	}
	if (PrintDebugInfo)
	{
		UKismetSystemLibrary::PrintString(GetWorld(), " RECOIL -> VERTICAL =  ", true, true);
		UKismetSystemLibrary::PrintString(GetWorld(), FString::SanitizeFloat(VerticalValue * VerticalRecoilMultiplier), true, true);
		UKismetSystemLibrary::PrintString(GetWorld(), " RECOIL -> HORIZONTAL = ", true, true);
		UKismetSystemLibrary::PrintString(GetWorld(), FString::SanitizeFloat(HorizontalValue * HorizontalRecoilMultiplier), true, true);
	}
}

void URecoilComponent::PlayCameraShake(TSubclassOf<class UCameraShakeBase> ShakeClass, float Scale)
{
	OwnerController->ClientStartCameraShake(ShakeClass, Scale);
}

void URecoilComponent::IncreaseRecoilMultiplier()
{
	if (MaxVerticalMultiplier > VerticalRecoilMultiplier)
	{
		VerticalRecoilMultiplier = VerticalRecoilMultiplier * VerticalRecoilRiseRate;
	}
	if (MaxHorizontalMultiplier > HorizontalRecoilMultiplier)
	{
		HorizontalRecoilMultiplier = HorizontalRecoilMultiplier * HorizontalRecoilRiseRate;
	}
}

void URecoilComponent::SetOwningController(APlayerController* NewController)
{
	OwnerController = NewController;
}
