// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "UObject/Object.h"
#include "SaveData.generated.h"

/**
 * 
 */

USTRUCT()
struct FPropertySaveData
{
	GENERATED_BODY()

	UPROPERTY(SaveGame)
	FName PropertyName;

	UPROPERTY(SaveGame)
	FString PropertyType;
	
	UPROPERTY(SaveGame)
	TArray<uint8> PropertyData;	
};

USTRUCT()
struct FComponentSaveData
{
	GENERATED_BODY()

	UPROPERTY(SaveGame)
	FString ComponentID;
	
	UPROPERTY(SaveGame)
	FString SaveVersion {"1.0.0"};
	
	UPROPERTY(SaveGame)
	TArray<FPropertySaveData> Properties;
	
	UPROPERTY(SaveGame)
	int32 PropertiesCount{0};
};

USTRUCT()
struct FObjectSaveData
{
	GENERATED_BODY()

	UPROPERTY(SaveGame)
	FString ObjectID;

	UPROPERTY(SaveGame)
	TSubclassOf<UObject> ObjectClass;

	UPROPERTY(SaveGame)
	FString SaveVersion {"1.0.0"};

	UPROPERTY(SaveGame)
	FTransform SpawnTransform;
	
	UPROPERTY(SaveGame)
	bool bIsDynamicSpawned {false};

	UPROPERTY(SaveGame)
	TArray<FPropertySaveData> Properties;

	UPROPERTY(SaveGame)
	int32 PropertiesCount{0};
	
	UPROPERTY(SaveGame)
	TArray<FComponentSaveData> Components;

	UPROPERTY(SaveGame)
	int32 ComponentsCount{0};
};


UCLASS()
class LOGICRAFTCOREUTILS_API ULCUSaveGame : public USaveGame
{
	GENERATED_BODY()
public:
	UPROPERTY(SaveGame)
	FString GlobalSaveVersion = "1.0.0";

	UPROPERTY(SaveGame)
	FDateTime SaveTimeStamp;

	UPROPERTY(SaveGame)
	FName SaveSlotName;

	UPROPERTY(SaveGame)
	TArray<FObjectSaveData> ObjectsData;	
};


