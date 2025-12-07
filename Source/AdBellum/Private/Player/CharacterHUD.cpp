// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/CharacterHUD.h"
#include "Blueprint/UserWidget.h"



void ACharacterHUD::HUDOpen(AActor* ControlledActor)
{
	OnHUDOpen(ControlledActor);
}

void ACharacterHUD::HUDClose()
{
	OnHUDClose();
}