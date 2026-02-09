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
	FString UniqueID;

	
public:
	void SetIsDynamicSpawned(TSubclassOf<UObject> SpawnActor, FGuid UID = FGuid::NewGuid());
	FString GetUniqueID() const;
	TSubclassOf<UObject> GetDynamicSpawnClass() const;
	bool GetIsDynamicSpawned() const;
	

	virtual void OnPreLoad();
	virtual void OnPreSave();
	
	virtual void OnPostLoad();
	virtual void OnPostSave();
};
