// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "RuntimePropertyEditorSettings.generated.h"

/**
 * 
 */
UCLASS(Config=Game)
class LOGICRAFTCOREUTILS_API URuntimePropertyEditorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	URuntimePropertyEditorSettings();
	
	UPROPERTY(Config, EditAnywhere)
	TSoftObjectPtr<UMaterialInstance> SelectionMaterial{ FSoftObjectPath{ "/LogicraftCoreUtils/Materials/RuntimePropertyEditor/Mat_Selection_Inst.Mat_Selection_Inst" } };

	UPROPERTY(Config, EditAnywhere)
	TSoftObjectPtr<UStaticMesh> SelectionMesh{ FSoftObjectPath{ "/Engine/BasicShapes/Cube.Cube" } };

	static const ThisClass* Get() { return GetDefault<URuntimePropertyEditorSettings>(); }
};
