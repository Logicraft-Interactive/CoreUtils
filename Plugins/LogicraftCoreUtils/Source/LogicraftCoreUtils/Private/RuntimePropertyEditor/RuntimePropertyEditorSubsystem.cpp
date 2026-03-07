// Copyright (c) Logicraft Interactive. All Rights Reserved.

#include "RuntimePropertyEditor/RuntimePropertyEditorSubsystem.h"

#include "RuntimePropertyEditor/RuntimePropertyEditorSettings.h"

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

void URuntimePropertyEditorSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	SpawnSelectionBox(InWorld);
}

void URuntimePropertyEditorSubsystem::OpenWindow()
{
	if (RuntimePropertyEditorWindow.IsValid())
	{
		return;
	}
	
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

void URuntimePropertyEditorSubsystem::RegisterEditableObject(const TScriptInterface<IRuntimeEditable>& RuntimeEditable)
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
	if (ensureMsgf(EditableObject.IsValid(), TEXT("Trying to use an invalid Editable Object")))
	{
		auto* EditableObjectRawPtr{ EditableObject.Get() };
	
		EditableObjectsUIProperties
			.Add(EditableObject, RuntimePropertyEditor->MakeEditablePropertiesWidget(EditableObjectRawPtr));
	
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
	if (SelectedActor.IsValid())
	{
		SelectedActor->GetRootComponent()->TransformUpdated.RemoveAll(this);
		SelectedActor.Reset();
		
		SelectionBox->SetActorHiddenInGame(true);
	}
	
	if (!SelectedItem.IsValid())
	{
		RuntimePropertyEditor->DisplayObjectProperties(nullptr);
		return;
	}

	if (AActor* Actor = Cast<AActor>(SelectedItem.Get()))
	{
		if (!SelectionBox.IsValid())
		{
			SpawnSelectionBox(*GetWorld());
		}
		
		SelectedActor = Actor;
		SelectedActor->GetRootComponent()->TransformUpdated.AddUObject(
			this, &URuntimePropertyEditorSubsystem::OnSelectedActorTransformUpdated);

		SelectionBox->SetActorTransform(Actor->GetTransform());
		SelectionBox->SetActorScale3D(SelectionBox->GetActorScale3D() + FVector{ 0.1 });
		SelectionBox->SetActorHiddenInGame(false);
	}

	if (const TSharedRef<SScrollBox>* PropertiesContainer{ EditableObjectsUIProperties.Find(SelectedItem) })
	{
		RuntimePropertyEditor->DisplayObjectProperties(*PropertiesContainer);
	}
}

void URuntimePropertyEditorSubsystem::OnSelectedActorTransformUpdated(USceneComponent* UpdatedComponent,
	EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport)
{
	if (!SelectionBox.IsValid())
	{
		SpawnSelectionBox(*GetWorld());
	}

	SelectionBox->SetActorTransform(UpdatedComponent->GetRelativeTransform());
	SelectionBox->SetActorScale3D(SelectionBox->GetActorScale3D() + FVector{ 0.1 });
}

void URuntimePropertyEditorSubsystem::SpawnSelectionBox(UWorld& InWorld)
{
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Name = TEXT("RuntimePropertyEditorSelectionBox");
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	SelectionBox = InWorld.SpawnActor<AActor>(SpawnParameters);
	
	SelectionBoxMesh = Cast<UStaticMeshComponent>(
		SelectionBox->AddComponentByClass(
				UStaticMeshComponent::StaticClass(),
				false,
				FTransform::Identity,
				false
		));

	auto* Settings{ URuntimePropertyEditorSettings::Get() };

	UStaticMesh* SelectionMesh = Settings->SelectionMesh.LoadSynchronous();
	UMaterialInstance* SelectionMaterial = Settings->SelectionMaterial.LoadSynchronous();
	
	SelectionBoxMesh->SetStaticMesh(SelectionMesh);
	SelectionBoxMesh->SetMaterial(0, SelectionMaterial);

	SelectionBox->SetActorHiddenInGame(true);
}
