// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterHUD.h"
#include "Blueprint/UserWidget.h"
#include "GunnerCrosshairWidget.h"

ACharacterHUD::ACharacterHUD()
{
	GunnerCrosshairWidgetClass = UGunnerCrosshairWidget::StaticClass();
}

void ACharacterHUD::HUDOpen(AActor* ControlledActor)
{
	OnHUDOpen(ControlledActor);
}

void ACharacterHUD::HUDClose()
{
	OnHUDClose();
}

void ACharacterHUD::UpdateGunnerCrosshair(EALSStationaryRole StationaryRole)
{
	const bool bShouldShow = StationaryRole == EALSStationaryRole::Gunner || StationaryRole == EALSStationaryRole::Gunner_Standing;
	if (!bShouldShow)
	{
		if (GunnerCrosshairWidget)
		{
			GunnerCrosshairWidget->RemoveFromParent();
		}
		return;
	}
	if (!GunnerCrosshairWidget && GunnerCrosshairWidgetClass)
	{
		GunnerCrosshairWidget = CreateWidget<UGunnerCrosshairWidget>(GetOwningPlayerController(), GunnerCrosshairWidgetClass);
	}
	if (GunnerCrosshairWidget)
	{
		GunnerCrosshairWidget->SetStationaryRole(StationaryRole);
		if (!GunnerCrosshairWidget->IsInViewport())
		{
			GunnerCrosshairWidget->AddToViewport();
		}
	}
}