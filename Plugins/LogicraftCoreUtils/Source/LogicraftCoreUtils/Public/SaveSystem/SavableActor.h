// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SavableActor.generated.h"

// This class does not need to be modified.
UINTERFACE()
class USavableActor : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class LOGICRAFTCOREUTILS_API ISavableActor
{
	GENERATED_BODY()

	bool bIsDynamicSpawned = false;
	TSubclassOf<AActor> DynamicSpawnClass = nullptr;
	FName UniqueID;

	using FPropertyTuple = TTuple<FName, FString, TArray<int32>>;
	
	TMap<FString, TFunction<FPropertyTuple(int, int ,int, FPropertyTuple)>> MigrateLogics;
	
public:
	void SetIsDynamicSpawned(TSubclassOf<AActor> SpawnActor, FGuid UID = FGuid::NewGuid());
	FName GetUniqueID() const;
	TSubclassOf<AActor> GetDynamicSpawnClass() const;
	bool GetIsDynamicSpawned() const;

	void AddMigrateVersionLogic(const FString& Version,
		const TFunction<FPropertyTuple(int,int,int, FPropertyTuple)>& MigrateLogic);
	void Migrate(int MajorVersion, int MinorVersion, int PatchVersion, FPropertyTuple Properties);

	virtual void OnPreLoad();
	virtual void OnPreSave();
	
	virtual void OnPostLoad();
	virtual void OnPostSave();
};
