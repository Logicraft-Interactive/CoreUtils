// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LogCategory.h"
#include "GameFramework/Actor.h"
#include "SaveSystem/SaveableActor.h"
#include "SaveSystem/SaveComponent.h"
#include "SaveSystem/SaveableComponent.h"
#include "SavableTestActor.generated.h"

UCLASS()
class USavableTestComponent : public USaveableComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame)
	int32 Ammo = 30;

	UPROPERTY(SaveGame)
	double Stamina = 87.5;

	virtual FString GetSaveVersion_Implementation() override { return TEXT("1.0.0"); }
	virtual void SetupSaveMigrateLogic_Implementation() override
	{
		AddMigrateDelegateLambda(TEXT("1.0.0"), TEXT("1.1.0"), [](USaveableComponent* Self, auto From, auto To, auto Property)
		{
			auto This = static_cast<USavableTestComponent*>(Self);
		});
	}
};

UCLASS()
class ASavableTestActor : public AActor, public ISaveableActor
{
	GENERATED_BODY()

public:
	ASavableTestActor()
	{
		PrimaryActorTick.bCanEverTick = false;
		SaveComponent = CreateDefaultSubobject<USaveComponent>(TEXT("SaveComponent"));
		SavableComponent = CreateDefaultSubobject<USavableTestComponent>(TEXT("SavableTestComponent"));
	}
	
	virtual void SetupSaveMigrateLogic_Implementation() override;

	UPROPERTY(SaveGame)
	int32 Health = 100;

	UPROPERTY(SaveGame)
	float Speed = 600.f;

	UPROPERTY(SaveGame)
	FString DisplayName = TEXT("BenchmarkActor");

	UPROPERTY(SaveGame)
	FName Tag = NAME_None;

	UPROPERTY(SaveGame)
	FVector Position = FVector::ZeroVector;

	UPROPERTY()
	TObjectPtr<USaveComponent> SaveComponent;

	UPROPERTY()
	TObjectPtr<USavableTestComponent> SavableComponent;
};

inline void ASavableTestActor::SetupSaveMigrateLogic_Implementation()
{
	ISaveableActor::SetupSaveMigrateLogic_Implementation();
}
