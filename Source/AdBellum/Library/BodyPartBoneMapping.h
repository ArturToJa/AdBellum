// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AdBellumEnumLibrary.h"
#include "BodyPartBoneMapping.generated.h"

USTRUCT(BlueprintType)
struct FBodyPartBoneMapping
{
	GENERATED_BODY()


	static EBodyPart GetBodyPartFromBoneName(FName BoneName)
	{
		static TMap<FName, EBodyPart> BoneMappingMap;
		if (BoneMappingMap.IsEmpty()) 
		{
			BoneMappingMap.Add(TEXT("head"), EBodyPart::HEAD);
			BoneMappingMap.Add(TEXT("neck_01"), EBodyPart::TORSO);
			BoneMappingMap.Add(TEXT("spine_01"), EBodyPart::TORSO);
			BoneMappingMap.Add(TEXT("spine_02"), EBodyPart::TORSO);
			BoneMappingMap.Add(TEXT("spine_03"), EBodyPart::TORSO);
			BoneMappingMap.Add(TEXT("pelvis"), EBodyPart::TORSO);
			BoneMappingMap.Add(TEXT("thigh_l"), EBodyPart::LEGS);
			BoneMappingMap.Add(TEXT("calf_l"), EBodyPart::LEGS);
			BoneMappingMap.Add(TEXT("foot_l"), EBodyPart::LEGS);
			BoneMappingMap.Add(TEXT("thigh_r"), EBodyPart::LEGS);
			BoneMappingMap.Add(TEXT("calf_r"), EBodyPart::LEGS);
			BoneMappingMap.Add(TEXT("foot_r"), EBodyPart::LEGS);
			BoneMappingMap.Add(TEXT("clavicle_l"), EBodyPart::ARMS);
			BoneMappingMap.Add(TEXT("upperarm_l"), EBodyPart::ARMS);
			BoneMappingMap.Add(TEXT("lowerarm_l"), EBodyPart::ARMS);
			BoneMappingMap.Add(TEXT("hand_l"), EBodyPart::ARMS);
			BoneMappingMap.Add(TEXT("clavicle_r"), EBodyPart::ARMS);
			BoneMappingMap.Add(TEXT("upperarm_r"), EBodyPart::ARMS);
			BoneMappingMap.Add(TEXT("lowerarm_r"), EBodyPart::ARMS);
			BoneMappingMap.Add(TEXT("hand_r"), EBodyPart::ARMS);
		}
		const EBodyPart* BodyPart = BoneMappingMap.Find(BoneName);
		// Search for the given BoneName in the BoneMappingMap and return the corresponding BodyPart
		if (BodyPart)
		{
			return *BodyPart;
		}
		// Return a default BodyPart if the BoneName is not found in the map
		return EBodyPart::EMPTY;
	}
};


