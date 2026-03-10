// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeEditable.h"

#include "RuntimePropertyHelper.generated.h"

/**
 * Blueprint Function Library providing convenient access to the Runtime Property Editor.
 * 
 * This class wraps the subsystem's functionality in Blueprint-callable static functions,
 * making it easier to control the editor from Blueprints, UI widgets, or C++.
 * 
 * All methods are static and use the world context to find the subsystem instance.
 * 
 * Usage from Blueprint:
 * - Call "Open Window" to show the editor
 * - Call "Close Window" to hide it
 * - Call "Register Editable Object" to add objects to the editor
 * 
 * Thread Safety: Not thread-safe. Must be called on the Game Thread.
 */
UCLASS()
class LOGICRAFTCOREUTILS_API URuntimePropertyHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Opens the runtime property editor window.
	 * 
	 * If the window is already open, this does nothing.
	 * 
	 * @param WorldContext - Any object with a valid UWorld (Actor, Component, GameMode, etc.)
	 * 
	 * Blueprint Example:
	 * - Node: "Open Window"
	 * - WorldContext: Self (from an Actor or Component)
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Property Editor", meta = (WorldContext = "WorldContext"))
	static void OpenWindow(const UObject* WorldContext);

	/**
	 * Closes the runtime property editor window.
	 * 
	 * If the window is already closed, this does nothing.
	 * Registered objects remain in memory and can be re-displayed if the window is reopened.
	 * 
	 * @param WorldContext - Any object with a valid UWorld
	 * 
	 * Blueprint Example:
	 * - Node: "Close Window"
	 * - WorldContext: Self
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Property Editor", meta = (WorldContext = "WorldContext"))
	static void CloseWindow(const UObject* WorldContext);

	/**
	 * Registers an object to appear in the runtime property editor.
	 * 
	 * The object must implement the IRuntimeEditable interface in C++.
	 * Blueprint-only objects cannot use this system (interface is not Blueprint-compatible).
	 * 
	 * @param WorldContext - Any object with a valid UWorld
	 * @param RuntimeEditable - The object to register (must implement IRuntimeEditable)
	 * 
	 * C++ Example:
	 * @code
	 * URuntimePropertyHelper::RegisterEditableObject(this, this);
	 * @endcode
	 * 
	 * Note: Not exposed to Blueprint because TScriptInterface cannot be properly
	 *       created in Blueprints for C++-only interfaces.
	 */
	static void RegisterEditableObject(const UObject* WorldContext, const TScriptInterface<IRuntimeEditable>& RuntimeEditable);
	
	/**
	 * Unregisters an object from the runtime property editor.
	 * 
	 * Removes the object from the list and clears its cached property widgets.
	 * If the object is currently selected, the properties panel is cleared.
	 * 
	 * @param WorldContext - Any object with a valid UWorld
	 * @param RuntimeEditable - The object to unregister
	 * 
	 * C++ Example:
	 * @code
	 * URuntimePropertyHelper::UnRegisterEditableObject(this, this);
	 * @endcode
	 * 
	 * Note: Not exposed to Blueprint (same reason as RegisterEditableObject).
	 */
	static void UnRegisterEditableObject(const UObject* WorldContext, const TScriptInterface<IRuntimeEditable>& RuntimeEditable);
};