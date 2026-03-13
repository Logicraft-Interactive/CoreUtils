// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SaveSystem/SavableObject.h"
#include "TestSavableActorComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOGICRAFTCOREUTILSSB_API UTestSavableActorComponent : public UActorComponent, public ISavableObject
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UTestSavableActorComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UPROPERTY(SaveGame, EditAnywhere)
	int StuffOne = 5000;

	UPROPERTY(SaveGame)
	double StuffTwo = 3516512.2151;

	virtual void SetupSaveMigrateLogic_Implementation() override;
public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
};
