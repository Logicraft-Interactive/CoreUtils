// Copyright (c) Logicraft Interactive. All Rights Reserved.

#include "RuntimePropertyEditor/RuntimePropertyEditorSubsystem.h"

URuntimePropertyEditorSubsystem::ThisClass* URuntimePropertyEditorSubsystem::Get(const UObject* WorldContext)
{
	if (const UWorld* World{ GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull) })
	{
		return World->GetSubsystem<ThisClass>();
	}

	return nullptr;
}

void URuntimePropertyEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	OpenWindow();
}

void URuntimePropertyEditorSubsystem::Deinitialize()
{
	Super::Deinitialize();

	CloseWindow();
}

void URuntimePropertyEditorSubsystem::OpenWindow()
{
	SAssignNew(RuntimePropertyEditorWindow, SWindow)
		.Title(FText::FromString(TEXT("RuntimePropertyEditor")))
			.ClientSize({ 1920.f / 2.f, 1080.f / 2.f })
			[
				SAssignNew(RuntimePropertyEditor, SRuntimePropertyEditor)
					.EditableObjectList(&EditableObjects)
					.OnEditableObjectAdded_UObject(this, &URuntimePropertyEditorSubsystem::OnEditableObjectAdded)
					.OnEditableObjectSelectionChanged_UObject(this, &URuntimePropertyEditorSubsystem::OnEditableObjectSelectionChanged)
			];

	if (RuntimePropertyEditorWindow.IsValid())
	{
		FSlateApplication::Get().AddWindow(RuntimePropertyEditorWindow.ToSharedRef());	
	}
}

void URuntimePropertyEditorSubsystem::CloseWindow()
{
	if (RuntimePropertyEditorWindow.IsValid())
	{
		RuntimePropertyEditorWindow->RequestDestroyWindow();	
	}
}

void URuntimePropertyEditorSubsystem::RegisterEditableProperties(const TScriptInterface<IRuntimeEditable>& RuntimeEditable)
{
	UObject* EditableObject{ RuntimeEditable.GetObject() };
	if (ensureMsgf(EditableObject || IsValid(EditableObject), TEXT("A nullptr runtime editable cannot be registered.")))
	{
		EditableObjects.Add(EditableObject);
	}
}

TSharedRef<ITableRow> URuntimePropertyEditorSubsystem::OnEditableObjectAdded(TWeakObjectPtr<> EditableObject,
	const TSharedRef<STableViewBase>& Owner)
{
	auto* EditableObjectRawPtr{ EditableObject.Get() };

	EditableObjectsUIProperties
		.Add(EditableObject, RuntimePropertyEditor->MakeEditablePropertiesScrollBox(EditableObjectRawPtr));
	
	return SNew(STableRow<TWeakObjectPtr<>>, Owner)
		[
			SNew(STextBlock).Text(FText::FromString(EditableObjectRawPtr->GetName()))
		];
}

void URuntimePropertyEditorSubsystem::OnEditableObjectSelectionChanged(TWeakObjectPtr<> SelectedItem,
	ESelectInfo::Type SelectInfo)
{
	if (!SelectedItem.IsValid())
	{
		RuntimePropertyEditor->DisplayPropertiesContainer(nullptr);
		return;
	}

	if (const TSharedRef<SScrollBox>* PropertiesContainer{ EditableObjectsUIProperties.Find(SelectedItem) })
	{
		RuntimePropertyEditor->DisplayPropertiesContainer(*PropertiesContainer);
	}
}
