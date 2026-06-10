// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimePropertyBuilder.h"
#include "UObject/Interface.h"
#include "RuntimeEditable.generated.h"

/**
 * UInterface wrapper for IRuntimeEditable.
 * This is required by Unreal's reflection system to make the interface visible to Blueprints and the engine.
 */
UINTERFACE(Category = "RuntimePropertyEditor")
class URuntimeEditable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface that allows any UObject to be displayed and edited in the Runtime Property Editor.
 * 
 * Objects implementing this interface can define which properties should be exposed
 * and how they should be displayed in the runtime UI.
 * 
 * @note This interface must be implemented in C++. Blueprint implementation is not supported.
 * 
 * Usage Example:
 * @code
 * class AMyActor : public AActor, public IRuntimeEditable
 * {
 *     GENERATED_BODY()
 *
 *	public:
 *		void ATestRuntimePropertyEditor::BeginPlay()
 *		{
 *			Super::BeginPlay();
 *
 *			URuntimePropertyHelper::RegisterEditableObject(this, this);
 *		}
 *
 *		void ATestRuntimePropertyEditor::EndPlay(const EEndPlayReason::Type EndPlayReason)
 *		{
 *			Super::EndPlay(EndPlayReason);
 *
 *			URuntimePropertyHelper::UnRegisterEditableObject(this, this);
 *		}
 * 
 *     virtual void OnPropertiesDisplay(FRuntimePropertyBuilder& Builder) override
 *     {
 *         Builder
 *             .AddCategory("Movement")
 *             .AddNumeric<float>("Speed", 
 *                 [this]() { return Speed; },
 *                 [this](float Val) { Speed = Val; })
 *             .AddButton("Reset", [this]() { ResetSpeed(); });
 *     }
 * 
 * private:
 *     float Speed = 100.0f;
 * };
 * @endcode
 */
class RUNTIMEPROPERTYEDITOR_API IRuntimeEditable
{
	GENERATED_BODY()

public:
	/**
	 * Called when the object is displayed in the Runtime Property Editor.
	 * Implement this method to define which properties and actions should be exposed in the UI.
	 * 
	 * @param PropertiesBuilder - Builder object providing a fluent API to add properties, buttons, categories, etc.
	 * 
	 * @note This method is called once when the object is first displayed.
	 *       If you need dynamic updates, consider rebuilding the UI or using reactive lambdas.
	 */
	virtual void OnPropertiesDisplay(FRuntimePropertyBuilder& PropertiesBuilder)
		PURE_VIRTUAL(IRuntimeEditable::OnPropertiesDisplay, )
};