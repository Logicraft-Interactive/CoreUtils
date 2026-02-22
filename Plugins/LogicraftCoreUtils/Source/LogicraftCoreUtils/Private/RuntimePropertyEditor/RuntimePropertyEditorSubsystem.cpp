// Copyright (c) Logicraft Interactive. All Rights Reserved.

#include "RuntimePropertyEditor/RuntimePropertyEditorSubsystem.h"
#include "Engine/GameEngine.h"

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

	EditableObjects = MakeShared<TArray<FEditableObjectType>>();

	//OpenWindow();
}

void URuntimePropertyEditorSubsystem::Deinitialize()
{
	Super::Deinitialize();

	CloseWindow();

	RuntimePropertyEditorWindow.Reset();
	RuntimePropertyEditor.Reset();
	EditableObjects.Reset();
	EditableObjectsUIProperties.Empty();
}

void URuntimePropertyEditorSubsystem::OpenWindow()
{
	SAssignNew(RuntimePropertyEditorWindow, SWindow)
		.Title(FText::FromString(TEXT("RuntimePropertyEditor")))
			.ClientSize({ 1920.f / 2.f, 1080.f / 2.f })
			[
				SAssignNew(RuntimePropertyEditor, SRuntimePropertyEditor)
					.EditableObjectList(EditableObjects.Get())
					.OnEditableObjectAdded_UObject(this, &URuntimePropertyEditorSubsystem::OnEditableObjectAdded)
					.OnEditableObjectSelectionChanged_UObject(this, &URuntimePropertyEditorSubsystem::OnEditableObjectSelectionChanged)
			];
	
	// UGameEngine* GameEngine = Cast<UGameEngine>(GEngine);
	// if (GameEngine && GameEngine->GameViewport && GameEngine->GameViewport->GetWindow().IsValid())
	// {
	// 	FSlateApplication::Get().AddWindowAsNativeChild(
	// 		RuntimePropertyEditorWindow.ToSharedRef(), 
	// 		GameEngine->GameViewport->GetWindow().ToSharedRef()
	// 	);
	// }
	
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
		EditableObjects->Add(EditableObject);
	}
}

TSharedRef<ITableRow> URuntimePropertyEditorSubsystem::OnEditableObjectAdded(TWeakObjectPtr<> EditableObject,
	const TSharedRef<STableViewBase>& Owner)
{
	if (ensureMsgf(EditableObject.IsValid(), TEXT("Trying to use an invalid Editable Object")))
	{
		auto* EditableObjectRawPtr{ EditableObject.Get() };
	
		EditableObjectsUIProperties
			.Add(EditableObject, RuntimePropertyEditor->MakeEditablePropertiesScrollBox(EditableObjectRawPtr));
	
		return SNew(STableRow<TWeakObjectPtr<>>, Owner)
			[
				SNew(STextBlock).Text(FText::FromString(EditableObjectRawPtr->GetName()))
			];
	}
	
	return SNew(STableRow<TWeakObjectPtr<>>, Owner)
		[
			SNew(STextBlock).Text(FText::FromString("Invalid Object"))
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
