// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LogCategory.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "SaveSystem/SavableActor.h"
#include "SaveSystem/SavableObject.h"
#include "SavableTestActor.generated.h"

UCLASS()
class USavableTestComponent : public UActorComponent, public ISavableObject
{
	GENERATED_BODY()

public:
	USavableTestComponent()
	{
		PrimaryComponentTick.bCanEverTick = false;
	}

	UPROPERTY(SaveGame)
	int32 Ammo = 30;

	UPROPERTY(SaveGame)
	double Stamina = 87.5;

	virtual FString GetVersion_Implementation() override { return TEXT("1.0.0"); }
	virtual void SetupSaveMigrateLogic_Implementation() override
	{
		AddMigrateDelegateLambda(TEXT("1.0.0"), TEXT("1.1.0"), [](UObject* Self, auto From, auto To, auto Property)
		{
			auto This = static_cast<USavableTestComponent*>(Self);
		});
	}
};

UCLASS()
class ASavableTestActor : public AActor, public ISavableActor
{
	GENERATED_BODY()

public:
	ASavableTestActor()
	{
		PrimaryActorTick.bCanEverTick = false;
		SavableComponent = CreateDefaultSubobject<USavableTestComponent>(TEXT("SavableTestComponent"));
	}

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
	TObjectPtr<USavableTestComponent> SavableComponent;

	virtual FString GetVersion_Implementation() override { return TEXT("1.0.0"); }

	virtual void SetupSaveMigrateLogic_Implementation() override
	{
		UE_LOG(LogSaveSystem, Log, TEXT("Setup Save Migrating"));
	}
};
