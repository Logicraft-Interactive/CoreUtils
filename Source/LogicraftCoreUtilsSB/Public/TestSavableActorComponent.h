// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SaveSystem/SaveableComponent.h"
#include "TestSavableActorComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOGICRAFTCOREUTILSSB_API UTestSavableActorComponent : public USaveableComponent
{
	GENERATED_BODY()

public:
	UTestSavableActorComponent();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(SaveGame, EditAnywhere)
	int StuffOne = 5000;

	UPROPERTY(SaveGame)
	double StuffTwo = 3516512.2151;

	virtual void SetupSaveMigrateLogic_Implementation() override;
public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
};
