// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimePropertyEditor/RuntimeEditable.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

/**
 * Delegate called when a new editable object is added to the list.
 * Responsible for generating the visual row representation in the ListView.
 * 
 * @param EditableObject - Weak pointer to the object being added
 * @param Owner - The ListView that owns this row
 * @return The table row widget to display for this object
 */
DECLARE_DELEGATE_RetVal_TwoParams(TSharedRef<ITableRow>, FOnEditableObjectAdded, TWeakObjectPtr<>, const TSharedRef<STableViewBase>&);

/**
 * Delegate called when the user selects a different object in the list.
 * 
 * @param SelectedItem - Weak pointer to the newly selected object (can be null if deselected)
 * @param SelectInfo - How the selection was made (direct click, keyboard, programmatic, etc.)
 */
DECLARE_DELEGATE_TwoParams(FOnEditableObjectSelectionChanged, TWeakObjectPtr<> SelectedItem, ESelectInfo::Type SelectInfo);

/**
 * Main Slate widget for the Runtime Property Editor.
 * 
 * This widget provides a two-panel interface:
 * - Left panel: Scrollable list of editable objects
 * - Right panel: Properties of the currently selected object
 * 
 * Layout structure:
 * @code
 * ┌────────────────────────────────────┐
 * │  SSplitter (Horizontal)            │
 * │  ┌──────────┬─────────────────┐    │
 * │  │ ListView │ Properties      │    │
 * │  │ (30%)    │ Panel (70%)     │    │
 * │  │          │                 │    │
 * │  │ Object1  │ ┌─────────────┐ │    │
 * │  │ Object2  │ │ Properties  │ │    │
 * │  │ Object3  │ │ ScrollBox   │ │    │
 * │  │          │ └─────────────┘ │    │
 * │  └──────────┴─────────────────┘    │
 * └────────────────────────────────────┘
 * @endcode
 * 
 * The panels are separated by a resizable splitter that the user can drag.
 * 
 * Architecture:
 * - This widget is purely UI (Slate) - it does not manage object lifecycle
 * - The URuntimePropertyEditorSubsystem owns the actual object data and handles callbacks
 * - This widget communicates with the subsystem via delegates
 * 
 * Thread Safety: Not thread-safe. Must be used on the Game Thread only.
 */
class LOGICRAFTCOREUTILS_API SRuntimePropertyEditor : public SCompoundWidget
{
	/** Type alias for list item sources (weak pointers to UObjects) */
	using FListItemSource = TWeakObjectPtr<>;
	
	/** Type alias for the ListView widget type */
	using SEditableObjectListView = SListView<FListItemSource>;
	
public:
	SLATE_BEGIN_ARGS(SRuntimePropertyEditor) {}

		/**
		 * Delegate that generates a table row for each editable object.
		 * Called by the ListView when an item needs to be displayed.
		 */
		SLATE_EVENT(FOnEditableObjectAdded, OnEditableObjectAdded)
		
		/**
		 * Delegate called when the user selects a different object.
		 * Used to update the right panel with the selected object's properties.
		 */
		SLATE_EVENT(FOnEditableObjectSelectionChanged, OnEditableObjectSelectionChanged)
		
		/**
		 * Pointer to the array of editable objects.
		 * This is owned by the subsystem and must remain valid for the widget's lifetime.
		 */
		SLATE_ARGUMENT(const TArray<FListItemSource>*, EditableObjectList)
		
	SLATE_END_ARGS()
		
	/**
	 * Constructs the widget hierarchy.
	 * 
	 * Creates a horizontal splitter with:
	 * - Left panel: ListView of editable objects
	 * - Right panel: Empty vertical box that will be filled with properties
	 * 
	 * @param InArgs - Widget construction arguments containing delegates and data
	 */
	void Construct(const FArguments& InArgs);

	/**
	 * Creates a scrollable container with all properties for a given editable object.
	 * 
	 * This method:
	 * 1. Creates a new SScrollBox
	 * 2. Passes it to the object's OnPropertiesDisplay() via FRuntimePropertyBuilder
	 * 3. Returns the populated scroll box
	 * 
	 * The returned scroll box is cached by the subsystem and displayed when this object is selected.
	 * 
	 * @param EditableProperties - The object implementing IRuntimeEditable
	 * @return A scroll box containing all the object's property widgets
	 * 
	 * @note This is called once per object when it's first added to the list.
	 *       The result is cached to avoid recreating widgets on every selection change.
	 */
	TSharedRef<SScrollBox> MakeEditablePropertiesWidget(const TScriptInterface<IRuntimeEditable>& EditableProperties);

	/**
	 * Forces the object list to refresh its visual representation.
	 * 
	 * Call this after modifying the underlying EditableObjectList array
	 * (adding/removing objects) to update the UI.
	 * 
	 * This is necessary because Slate doesn't automatically detect changes
	 * to the source array - it must be explicitly notified.
	 */
	void RefreshEditableObjectList();
	
	/**
	 * Displays the properties of a specific object in the right panel.
	 * 
	 * Clears the right panel and replaces it with the provided properties container.
	 * Pass nullptr to clear the panel (e.g., when no object is selected).
	 * 
	 * @param PropertiesContainer - The scroll box containing the object's properties, or nullptr to clear
	 */
	void DisplayObjectProperties(const TSharedPtr<SScrollBox>& PropertiesContainer);

	/**
	 * Checks if the given properties container is currently displayed.
	 * 
	 * Used by the subsystem to determine if the right panel needs to be cleared
	 * when an object is unregistered.
	 * 
	 * @param SelectedScrollBox - The scroll box to check
	 * @return true if this scroll box is currently displayed in the right panel
	 */
	bool IsSelected(const TSharedPtr<SScrollBox>& SelectedScrollBox) const;

private:
	/** 
	 * The ListView widget displaying all editable objects.
	 * Users can click on items in this list to select them.
	 */
	TSharedPtr<SEditableObjectListView> EditableObjectList;
	
	/**
	 * The right panel container.
	 * This vertical box is populated with the selected object's properties scroll box.
	 */
	TSharedPtr<SVerticalBox> EditablePropertiesPanel;
	
	/**
	 * Reference to the currently displayed properties scroll box.
	 * Used to detect if we need to clear the panel when an object is unregistered.
	 */
	TSharedPtr<SScrollBox> SelectedObjectScrollBox;
};