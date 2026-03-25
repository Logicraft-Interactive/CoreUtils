// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SaveSystem/SaveComponent.h"
#include "TestSavableActor.generated.h"

class UTestSavableActorComponent;

UCLASS()
class LOGICRAFTCOREUTILSSB_API UTestSavableComponent_Save : public USaveComponent
{
	GENERATED_BODY()

public:
	virtual FString GetSaveVersion_Implementation() override;
};

UCLASS()
class LOGICRAFTCOREUTILSSB_API ATestSavableActor : public AActor
{
	GENERATED_BODY()

public:
	ATestSavableActor();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(SaveGame)
	int Life = 50;

	UPROPERTY(SaveGame)
	FName Name = "Test";

	UPROPERTY(EditAnywhere)
	TObjectPtr<UTestSavableActorComponent> SavableActorComponent;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UTestSavableComponent_Save> SaveComponent;

	UPROPERTY(EditAnywhere)
	bool bDynamicSpawned = false;

public:
	virtual void Tick(float DeltaTime) override;
};