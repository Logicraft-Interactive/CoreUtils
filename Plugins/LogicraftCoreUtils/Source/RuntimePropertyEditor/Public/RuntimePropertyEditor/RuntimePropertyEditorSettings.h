// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "RuntimePropertyEditorSettings.generated.h"

class UMaterialInstance;
class UStaticMesh;

/**
 * Project settings for the Runtime Property Editor.
 * 
 * Accessible via Project Settings > Plugins > Runtime Property Editor
 * 
 * These settings configure the visual appearance of the selection box
 * that appears around selected actors in the editor.
 * 
 * Default values:
 * - SelectionMaterial: Wireframe material from the plugin's content folder
 * - SelectionMesh: Engine's default cube mesh
 * 
 * Thread Safety: Read-only after initialization. Safe to access from any thread.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Runtime Property Editor"))
class RUNTIMEPROPERTYEDITOR_API URuntimePropertyEditorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/**
	 * Material used for the selection box that appears around selected actors.
	 * 
	 * Should be a translucent or wireframe material for best visual effect.
	 * 
	 * Default: /LogicraftCoreUtils/Materials/RuntimePropertyEditor/Mat_Selection_Inst
	 * 
	 * Configurable in: Project Settings > Plugins > Runtime Property Editor
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Visual Feedback", meta = (AllowedClasses = "/Script/Engine.MaterialInstance"))
	TSoftObjectPtr<UMaterialInstance> SelectionMaterial{ 
		FSoftObjectPath{ "/LogicraftCoreUtils/Materials/RuntimePropertyEditor/Mat_Selection_Inst.Mat_Selection_Inst" }
	};

	/**
	 * Static mesh used for the selection box.
	 * 
	 * Should be a simple geometric shape (cube, sphere, etc.).
	 * 
	 * Default: Engine's basic cube shape (/Engine/BasicShapes/Cube)
	 * 
	 * Configurable in: Project Settings > Plugins > Runtime Property Editor
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Visual Feedback", meta = (AllowedClasses = "/Script/Engine.StaticMesh"))
	TSoftObjectPtr<UStaticMesh> SelectionMesh{ FSoftObjectPath{ "/Engine/BasicShapes/Cube.Cube" }
	};

	/**
	 * Gets the singleton settings instance.
	 * 
	 * @return Pointer to the settings object
	 * 
	 * Usage:
	 * @code
	 * const auto* Settings = URuntimePropertyEditorSettings::Get();
	 * UStaticMesh* Mesh = Settings->SelectionMesh.LoadSynchronous();
	 * @endcode
	 */
	static const ThisClass* Get() { return GetDefault<URuntimePropertyEditorSettings>(); }
};