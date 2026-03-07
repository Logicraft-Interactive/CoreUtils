// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeEditable.h"
#include "RuntimePropertyEditor.h"
#include "Subsystems/WorldSubsystem.h"
#include "RuntimePropertyEditorSubsystem.generated.h"

/**
 * 
 */
UCLASS(Category = "RuntimePropertyEditor")
class LOGICRAFTCOREUTILS_API URuntimePropertyEditorSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
private:
	using FEditableObjectType = TWeakObjectPtr<UObject>;
	
	TSharedPtr<SWindow> RuntimePropertyEditorWindow;
	TSharedPtr<SRuntimePropertyEditor> RuntimePropertyEditor;

	TArray<FEditableObjectType> EditableObjects;
	TMap<FEditableObjectType, TSharedRef<SScrollBox>> EditableObjectsUIProperties;

	TWeakObjectPtr<AActor> SelectionBox;
	TWeakObjectPtr<UStaticMeshComponent> SelectionBoxMesh;
	TWeakObjectPtr<AActor> SelectedActor;
public:
	static ThisClass* Get(const UObject* WorldContext);
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	
	UFUNCTION(BlueprintCallable)
	void OpenWindow();

	UFUNCTION(BlueprintCallable)
	void CloseWindow();
	
	void RegisterEditableObject(const TScriptInterface<IRuntimeEditable>& RuntimeEditable);

private:
	TSharedRef<ITableRow> OnEditableObjectAdded(TWeakObjectPtr<> EditableObject, const TSharedRef<STableViewBase>& Owner);

	void OnEditableObjectSelectionChanged(TWeakObjectPtr<> SelectedItem, ESelectInfo::Type SelectInfo);

	void OnSelectedActorTransformUpdated(USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport);

	void SpawnSelectionBox(UWorld& InWorld);

};
