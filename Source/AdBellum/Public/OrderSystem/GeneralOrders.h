// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

template<OrderEnum Order>
struct ADBELLUM_API GeneralOrder;

#include "OrderSystem/PatrolOrder.h"
#include "OrderSystem/StopOrder.h"
#include "OrderSystem/TakeCoverOrder.h"
#include "OrderSystem/MoveToLocationOrder.h"
#include "OrderSystem/OccupyAreaOfInterestOrder.h"
#include "OrderSystem/FollowUnitOrder.h"
#include "OrderSystem/AttackUnitOrder.h"
#include "OrderSystem/EnterOrder.h"
#include "OrderSystem/InteractOrder.h"
#include "OrderSystem/HoldPositionOrder.h"
#include "OrderSystem/ReloadOrder.h"
#include "OrderSystem/TrainingOrder.h"
