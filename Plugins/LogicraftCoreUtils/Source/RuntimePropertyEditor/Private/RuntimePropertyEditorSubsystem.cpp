// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#include "RuntimePropertyEditorSubsystem.h"

#include "LogCategory.h"
#include "RuntimePropertyEditorSettings.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstance.h"
#include "Components/StaticMeshComponent.h"
#include "Widgets/Layout/SScrollBox.h"

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

	if (SelectionBox.IsValid())
	{
		SelectionBox->Destroy();
		SelectionBox.Reset();
	}
	
	EditableObjects.Empty();
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

		RuntimePropertyEditorWindow->GetOnWindowClosedEvent()
			.AddUObject(this, &URuntimePropertyEditorSubsystem::OnWindowCloseEvent);
	}
}

void URuntimePropertyEditorSubsystem::CloseWindow()
{
	if (RuntimePropertyEditorWindow.IsValid())
	{
		RuntimePropertyEditorWindow->RequestDestroyWindow();
		RuntimePropertyEditorWindow.Reset();
		RuntimePropertyEditor.Reset();

		constexpr bool IsHidden{ true };
		HideSelectionBox(IsHidden, nullptr);
	}
}

void URuntimePropertyEditorSubsystem::RegisterEditableObject(const TScriptInterface<IRuntimeEditable>& RuntimeEditable)
{
	UObject* EditableObject{ RuntimeEditable.GetObject() };
	if (ensureMsgf(EditableObject || IsValid(EditableObject),
		TEXT("URuntimePropertyEditorSubsystem: A nullptr runtime editable cannot be registered.")))
	{
		EditableObjects.Add(EditableObject);
	}
}

void URuntimePropertyEditorSubsystem::UnRegisterEditableObject(
	const TScriptInterface<IRuntimeEditable>& RuntimeEditable)
{
	UObject* RuntimeEditableObject{ RuntimeEditable.GetObject() };
	
	if (!ensureMsgf(IsValid(RuntimeEditableObject),
		TEXT("URuntimePropertyEditorSubsystem: Unable to unregister an invalid UObject.")))
	{
		return;
	}

	EditableObjects.Remove(RuntimeEditableObject);

	const TSharedRef RemovedScrollBox{ EditableObjectsUIProperties[RuntimeEditableObject] };
	EditableObjectsUIProperties.Remove(RuntimeEditableObject);
	
	if (RuntimePropertyEditor.IsValid() && RuntimePropertyEditor->IsSelected(RemovedScrollBox))
	{
		RuntimePropertyEditor->DisplayObjectProperties(nullptr);
		
		constexpr bool IsHidden{ true };
		HideSelectionBox(IsHidden, nullptr);
	}

	RuntimePropertyEditor->RefreshEditableObjectList();
}

TSharedRef<ITableRow> URuntimePropertyEditorSubsystem::OnEditableObjectAdded(TWeakObjectPtr<> EditableObject,
                                                                             const TSharedRef<STableViewBase>& Owner)
{
	if (ensureMsgf(EditableObject.IsValid(),
		TEXT("URuntimePropertyEditorSubsystem: A nullptr runtime editable cannot be registered.")))
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
		constexpr bool IsHidden{ true };
		HideSelectionBox(IsHidden, nullptr);
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
		
		constexpr bool IsHidden{ false };
		HideSelectionBox(IsHidden, Actor);
	}

	if (const TSharedRef<SScrollBox>* PropertiesContainer{ EditableObjectsUIProperties.Find(SelectedItem) })
	{
		RuntimePropertyEditor->DisplayObjectProperties(*PropertiesContainer);
	}
}

void URuntimePropertyEditorSubsystem::OnSelectedActorTransformUpdated(USceneComponent* UpdatedComponent,
	EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport)
{	
	if (!SelectionBox.IsValid() || !UpdatedComponent)
	{
		return;
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
	
	if (!SelectionBox.IsValid())
	{
		UE_LOG(LogRPE, Error, TEXT("RuntimePropertyEditorSubsystem: Failed to spawn selection box!"));
		return;
	}
	
	SelectionBoxMesh = Cast<UStaticMeshComponent>(
		SelectionBox->AddComponentByClass(
			UStaticMeshComponent::StaticClass(),
			false,
			FTransform::Identity,
			false
		));

	if (!SelectionBoxMesh.IsValid())
	{
		UE_LOG(LogRPE, Error, TEXT("URuntimePropertyEditorSubsystem: Failed to create selection box mesh!"));
		return;
	}

	const URuntimePropertyEditorSettings* Settings = URuntimePropertyEditorSettings::Get();
    
	if (UStaticMesh* SelectionMesh = Settings->SelectionMesh.LoadSynchronous())
	{
		SelectionBoxMesh->SetStaticMesh(SelectionMesh);
	}
    
	if (UMaterialInstance* SelectionMaterial = Settings->SelectionMaterial.LoadSynchronous())
	{
		SelectionBoxMesh->SetMaterial(0, SelectionMaterial);
	}

	SelectionBox->SetActorHiddenInGame(true);
}

void URuntimePropertyEditorSubsystem::HideSelectionBox(bool bIsHidden, AActor* ReferenceActor)
{
	if (!SelectionBox.IsValid())
	{
		return;
	}
	
	if (bIsHidden)
	{
		if (SelectedActor.IsValid())
		{
			SelectedActor->GetRootComponent()->TransformUpdated.RemoveAll(this);
			SelectedActor.Reset();	
		}
		
		SelectionBox->SetActorHiddenInGame(true);

		return;
	}

	SelectedActor = ReferenceActor;
	SelectedActor->GetRootComponent()->TransformUpdated.AddUObject(
	this, &URuntimePropertyEditorSubsystem::OnSelectedActorTransformUpdated);

	SelectionBox->SetActorTransform(SelectedActor->GetTransform());

	constexpr float SelectionBoxPadding = 0.1f;
	SelectionBox->SetActorScale3D(SelectionBox->GetActorScale3D() + FVector{ SelectionBoxPadding });
	SelectionBox->SetActorHiddenInGame(false);
}

void URuntimePropertyEditorSubsystem::OnWindowCloseEvent(const TSharedRef<SWindow>&)
{
	if (RuntimePropertyEditor.IsValid())
	{
		RuntimePropertyEditor->DisplayObjectProperties(nullptr);
	}
	
	RuntimePropertyEditorWindow.Reset();
	RuntimePropertyEditor.Reset();

	constexpr bool IsHidden{ true };
	HideSelectionBox(IsHidden, nullptr);
}
