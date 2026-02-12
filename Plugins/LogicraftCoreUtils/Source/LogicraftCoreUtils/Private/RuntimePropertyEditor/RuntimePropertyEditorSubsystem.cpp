// Copyright (c) Logicraft Interactive. All Rights Reserved.

#include "RuntimePropertyEditor/RuntimePropertyEditorSubsystem.h"

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

void URuntimePropertyEditorSubsystem::RegisterEditableProperties(TScriptInterface<IRuntimeEditable> EditableProperties)
{
	UObject* EditableObject{ EditableProperties.GetObject() };
	if (ensureMsgf(EditableObject || IsValid(EditableObject), TEXT("A null EditableObject cannot be registered.")))
	{
		RuntimePropertyEditor->AddEditableProperties(EditableObject);
	}
}
