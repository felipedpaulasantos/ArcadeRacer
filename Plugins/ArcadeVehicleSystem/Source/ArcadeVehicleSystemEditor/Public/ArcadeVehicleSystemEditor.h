/** Created and owned by Furious Production LTD @ 2023. **/

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FArcadeVehicleSystemEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
