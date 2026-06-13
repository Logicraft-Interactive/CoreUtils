// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeEditable.h"
#include "SRuntimePropertyEditor.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/EngineTypes.h"
#include "Components/SceneComponent.h"
#include "RuntimePropertyEditorSubsystem.generated.h"

/**
 * World subsystem managing the Runtime Property Editor lifecycle and state.
 * 
 * Responsibilities:
 * - Owns the editor window and all registered editable objects
 * - Manages object registration/unregistration
 * - Handles selection changes and UI updates
 * - Provides visual feedback via a selection box in the world
 * 
 * Lifetime:
 * - Created when a UWorld is created
 * - Destroyed when the UWorld is destroyed
 * - One instance per UWorld (multiplayer: one per client + one on server)
 * 
 * Usage:
 * @code
 * // Get the subsystem
 * auto* Subsystem = URuntimePropertyEditorSubsystem::Get(this);
 * 
 * // Open the editor window
 * Subsystem->OpenWindow();
 * 
 * // Register an object
 * Subsystem->RegisterEditableObject(MyActor);
 * 
 * // Register an object
 * Subsystem->UnRegisterEditableObject(MyActor);
 * @endcode
 * 
 * Thread Safety: Not thread-safe. All methods must be called on the Game Thread.
 */
UCLASS(Category = "RuntimePropertyEditor")
class RUNTIMEPROPERTYEDITOR_API URuntimePropertyEditorSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
private:
	/** Type alias for weak pointers to editable objects */
	using FEditableObjectType = TWeakObjectPtr<UObject>;
	
	/** 
	 * The main editor window.
	 * Created when OpenWindow() is called, destroyed when CloseWindow() is called.
	 */
	TSharedPtr<SWindow> RuntimePropertyEditorWindow;
	
	/**
	 * The Slate widget containing the editor UI.
	 * Lives inside RuntimePropertyEditorWindow.
	 */
	TSharedPtr<SRuntimePropertyEditor> RuntimePropertyEditor;

	/**
	 * Array of all registered editable objects.
	 * This is the data source for the ListView in the editor.
	 * 
	 * Using TWeakObjectPtr ensures we don't prevent objects from being garbage collected.
	 */
	TArray<FEditableObjectType> EditableObjects;
	
	/**
	 * Cache mapping objects to their property UI widgets.
	 * 
	 * Key: Weak pointer to the object
	 * Value: Scroll box containing that object's properties
	 * 
	 * This cache avoids recreating property widgets every time an object is selected.
	 * Widgets are created once when the object is registered and reused thereafter.
	 */
	TMap<FEditableObjectType, TSharedRef<SScrollBox>> EditableObjectsUIProperties;

	/**
	 * Visual selection box actor spawned in the world.
	 * Rendered around the currently selected actor to provide visual feedback.
	 */
	TWeakObjectPtr<AActor> SelectionBox;
	
	/**
	 * Static mesh component of the selection box.
	 * Used to set the mesh and material.
	 */
	TWeakObjectPtr<UStaticMeshComponent> SelectionBoxMesh;
	
	/**
	 * Currently selected actor in the editor.
	 * Used to track transform changes and update the selection box position.
	 */
	TWeakObjectPtr<AActor> SelectedActor;

public:
	/**
	 * Gets the subsystem instance for a given world context.
	 * 
	 * @param WorldContext - Any UObject that can provide a UWorld (Actor, Component, etc.)
	 * @return The subsystem instance, or nullptr if the world is invalid
	 * 
	 * Example:
	 * @code
	 * auto* Subsystem = URuntimePropertyEditorSubsystem::Get(this);
	 * if (Subsystem)
	 * {
	 *     Subsystem->OpenWindow();
	 * }
	 * @endcode
	 */
	static ThisClass* Get(const UObject* WorldContext);
	
	/**
	 * Called when the subsystem is created.
	 * Initializes the subsystem but does NOT open the window automatically.
	 * 
	 * @param Collection - Collection of all subsystems
	 */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	/**
	 * Called when the subsystem is destroyed (world cleanup).
	 * 
	 * Responsibilities:
	 * - Close the editor window if open
	 * - Destroy the selection box
	 * - Clear all cached data
	 */
	virtual void Deinitialize() override;

	/**
	 * Called when gameplay begins (after BeginPlay on all actors).
	 * Spawns the selection box actor.
	 * 
	 * @param InWorld - The world that just started gameplay
	 */
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	
	/**
	 * Opens the runtime property editor window.
	 * 
	 * If the window is already open, this method does nothing (prevents duplicates).
	 * 
	 * The window is created as a native OS window (not an in-game widget)
	 * and can be moved/resized like any application window.
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Property Editor")
	void OpenWindow();

	/**
	 * Closes the runtime property editor window.
	 * 
	 * Also hides the selection box and clears all UI state.
	 * Registered objects are NOT unregistered - they remain in memory.
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Property Editor")
	void CloseWindow();
	
	/**
	 * Registers an object to be editable in the runtime editor.
	 * 
	 * The object must implement the IRuntimeEditable interface.
	 * 
	 * @param RuntimeEditable - Object to register (must be valid and implement IRuntimeEditable)
	 * 
	 * Note: Objects are NOT automatically unregistered when destroyed.
	 * Call UnRegisterEditableObject() explicitly if needed.
	 */
	void RegisterEditableObject(const TScriptInterface<IRuntimeEditable>& RuntimeEditable);
	
	/**
	 * Unregisters an object from the runtime editor.
	 * 
	 * Removes the object from the list and clears its cached property widgets.
	 * If the object is currently selected, the properties panel is cleared.
	 * 
	 * @param RuntimeEditable - Object to unregister
	 */
	void UnRegisterEditableObject(const TScriptInterface<IRuntimeEditable>& RuntimeEditable);

private:
	/**
	 * Delegate handler: Generates a table row for an object in the ListView.
	 * 
	 * Called by the ListView when it needs to display an object.
	 * Creates the property cache for this object if it doesn't exist yet.
	 * 
	 * @param EditableObject - The object to create a row for
	 * @param Owner - The ListView that owns this row
	 * @return A table row widget displaying the object's name
	 */
	TSharedRef<ITableRow> OnEditableObjectAdded(TWeakObjectPtr<> EditableObject, const TSharedRef<STableViewBase>& Owner);

	/**
	 * Delegate handler: Called when the user selects a different object.
	 * 
	 * Updates the right panel to show the selected object's properties.
	 * If the object is an Actor, shows the selection box around it.
	 * 
	 * @param SelectedItem - The newly selected object (can be null)
	 * @param SelectInfo - How the selection was made
	 */
	void OnEditableObjectSelectionChanged(TWeakObjectPtr<> SelectedItem, ESelectInfo::Type SelectInfo);

	/**
	 * Delegate handler: Called when the selected actor's transform changes.
	 * 
	 * Updates the selection box position/scale to match the actor.
	 * 
	 * @param UpdatedComponent - The component whose transform changed
	 * @param UpdateTransformFlags - Flags describing the transform change
	 * @param Teleport - Whether this was a teleport (instant move)
	 */
	void OnSelectedActorTransformUpdated(USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport);

	/**
	 * Spawns the selection box actor in the world.
	 * 
	 * The selection box is a cube mesh with a special material that renders
	 * around the selected actor to provide visual feedback.
	 * 
	 * @param InWorld - The world to spawn the selection box in
	 */
	void SpawnSelectionBox(UWorld& InWorld);

	/**
	 * Shows or hides the selection box.
	 * 
	 * When showing, the box is positioned around ReferenceActor and follows its transform.
	 * When hiding, transform tracking is stopped and the box is hidden.
	 * 
	 * @param bIsHidden - true to hide, false to show
	 * @param ReferenceActor - The actor to surround with the box (null when hiding)
	 */
	void HideSelectionBox(bool bIsHidden, AActor* ReferenceActor);

	/**
	 * Delegate handler: Called when the user closes the window via the OS close button.
	 * 
	 * Cleans up window references and hides the selection box.
	 * 
	 * @param ClosedWindow - The window that was closed
	 */
	void OnWindowCloseEvent(const TSharedRef<SWindow>& ClosedWindow);
};