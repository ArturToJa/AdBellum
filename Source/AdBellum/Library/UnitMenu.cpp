// Fill out your copyright notice in the Description page of Project Settings.


#include "UnitMenu.h"


FBodyPartIndexStructSecond::FBodyPartIndexStructSecond()
	: bodyPart(BodyPartEnumSecond::EMPTY)
	, mainIndex(0)
	, maxIndex(0)
	, materialIndex(0)
	, maxMaterialIndex(0)
{}

FBodyPartIndexStructSecond::FBodyPartIndexStructSecond(BodyPartEnumSecond _bodyPart, int32 _mainIndex, int32 _maxIndex, int32 _materialIndex, int32 _maxMaterialIndex)
	: bodyPart(_bodyPart)
	, mainIndex(_mainIndex)
	, maxIndex(_maxIndex)
	, materialIndex(_materialIndex)
	, maxMaterialIndex(_maxMaterialIndex)
{}