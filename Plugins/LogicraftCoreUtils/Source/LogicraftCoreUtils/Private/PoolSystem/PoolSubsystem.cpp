// Copyright (c) Logicraft Interactive. All Rights Reserved.


#include "PoolSystem/PoolSubsystem.h"

#include "PoolSystem/PoolObject.h"

void UPoolSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UPoolSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

UPoolObject* UPoolSubsystem::CreatePool(FPoolSettings PoolSettings)
{	
	PoolSettings.WorldContext = GetWorld();
	UPoolObject* NewPoolObject = NewObject<UPoolObject>(this);
	NewPoolObject->SetupPoolObject(PoolSettings);

	return NewPoolObject;
}

UPoolObject* UPoolSubsystem::CreatePoolFromDataAsset(UClass* SpawnClass, UPoolSettingsDataAsset* PoolSettings)
{
	if (!PoolSettings)
	{
		ensureMsgf(false, TEXT("UPoolSubsystem::CreatePoolFromDataAsset called with null PoolSettings data asset."));
		return nullptr;
	}
	FPoolSettings NewPoolSettings = PoolSettings->PoolSettings;
	NewPoolSettings.SpawnClass = SpawnClass;
	NewPoolSettings.WorldContext = GetWorld();
	
	UPoolObject* NewPoolObject = NewObject<UPoolObject>(this);
	NewPoolObject->SetupPoolObject(NewPoolSettings);

	return NewPoolObject;
}
