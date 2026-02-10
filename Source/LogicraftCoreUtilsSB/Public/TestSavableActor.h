// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SaveSystem/SavableActor.h"
#include "TestSavableActor.generated.h"

class UTestSavableActorComponent;


UCLASS()
class LOGICRAFTCOREUTILSSB_API ATestSavableActor : public AActor, public ISavableActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ATestSavableActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(SaveGame)
	int Life = 50;

	UPROPERTY(SaveGame)
	FName Name = "Test";

	UPROPERTY(EditAnywhere)
	TObjectPtr<UTestSavableActorComponent> SavableActorComponent;

	UPROPERTY(EditAnywhere)
	bool bDynamicSpawned = false;
	
	virtual FString GetVersion() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
