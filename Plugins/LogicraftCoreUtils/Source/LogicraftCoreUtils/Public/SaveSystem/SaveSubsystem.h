// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SaveData.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SaveSubsystem.generated.h"

/**
 * 
 */

struct FSaveSerializer
{
	FObjectSaveData SerializeObject(UObject* Object);
};


UCLASS()
class LOGICRAFTCOREUTILS_API USaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	using VersionType = TTuple<int, int, int>;
	FSaveData CurrentSave;
	
	TOptional<VersionType> ExtractVersion(const FString& Version);
	
	
public:

	void SaveWorld(const FName& SlotName, const FString& Version);	
};
