// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
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
struct FObjectSaveData
{
	GENERATED_BODY()

	UPROPERTY(SaveGame)
	FString ObjectID;

	UPROPERTY(SaveGame)
	TSubclassOf<UObject> ObjectClass;

	UPROPERTY(SaveGame)
	int32 SaveVersion = 1;

	UPROPERTY(SaveGame)
	TArray<FObjectSaveData> Properties;
};


USTRUCT()
struct LOGICRAFTCOREUTILS_API FSaveData
{
	GENERATED_BODY()

	UPROPERTY(SaveGame)
	FString GlobalSaveVersion = "1.0.0";

	UPROPERTY(SaveGame)
	FDateTime SaveTimeStamp;

	UPROPERTY(SaveGame)
	FName SaveSlotName;

	UPROPERTY(SaveGame)
	TArray<FObjectSaveData> ObjectsData;	
};


