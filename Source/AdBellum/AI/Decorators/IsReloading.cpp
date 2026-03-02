// Fill out your copyright notice in the Description page of Project Settings.


#include "IsReloading.h"
#include "AIController.h"
#include "Unit/ArmedUnitInterface.h"
#include "Weapon/IWeapon.h"


bool UIsReloading::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		if (APawn* OwningPawn = AIController->GetPawn())
		{
			if (AActor* Weapon = IArmedUnitInterface::Execute_GetWeapon(OwningPawn))
			{
				return IIWeapon::Execute_IsReloading(Weapon);
			}
			return false;
		}
	}
	return false;
}