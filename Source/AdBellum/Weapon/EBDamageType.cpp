// Fill out your copyright notice in the Description page of Project Settings.


#include "EBDamageType.h"

UEBDamageType::UEBDamageType() 
{
	
}

EBodyPart UEBDamageType::GetBodyPart()
{
	return BodyPart;
}

void UEBDamageType::SetBodyPart(EBodyPart InBodyPart)
{
	BodyPart = InBodyPart;
}

UEBDamageTypeHEAD::UEBDamageTypeHEAD() 
{
	BodyPart = EBodyPart::HEAD;
}

UEBDamageTypeTORSO::UEBDamageTypeTORSO()
{
	BodyPart = EBodyPart::TORSO;
}

UEBDamageTypeARMS::UEBDamageTypeARMS()
{
	BodyPart = EBodyPart::ARMS;
}

UEBDamageTypeLEGS::UEBDamageTypeLEGS()
{
	BodyPart = EBodyPart::LEGS;
}
