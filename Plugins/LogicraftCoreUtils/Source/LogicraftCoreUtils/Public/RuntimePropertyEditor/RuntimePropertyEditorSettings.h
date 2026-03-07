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
	UPROPERTY(Config, EditAnywhere)
	TSoftObjectPtr<UMaterialInstance> SelectionMaterial;

	UPROPERTY(Config, EditAnywhere)
	TSoftObjectPtr<UStaticMesh> SelectionMesh;

	static const ThisClass* Get() { return GetDefault<URuntimePropertyEditorSettings>(); }
};
