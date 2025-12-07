// Designed by Hitman's Store, 2023

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FRecoilAnimationModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
