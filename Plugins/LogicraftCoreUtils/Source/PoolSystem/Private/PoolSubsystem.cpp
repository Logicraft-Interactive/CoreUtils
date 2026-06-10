// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#include "PoolSubsystem.h"
#include "PoolObject.h"
#include "Engine/World.h"

void UPoolSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UPoolSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

UPoolObject* UPoolSubsystem::CreatePool(FPoolSettings PoolSettings, UObject* PoolOwner)
{
	PoolSettings.WorldContext = GetWorld();
	UPoolObject* NewPoolObject = NewObject<UPoolObject>(PoolOwner);
	NewPoolObject->SetupPoolObject(PoolSettings);

	return NewPoolObject;
}

UPoolObject* UPoolSubsystem::CreatePoolFromDataAsset(UPoolSettingsDataAsset* PoolSettings, UObject* PoolOwner)
{
	if (!PoolSettings)
	{
		ensureMsgf(false, TEXT("UPoolSubsystem::CreatePoolFromDataAsset called with null PoolSettings data asset."));
		return nullptr;
	}
	
	FPoolSettings NewPoolSettings = PoolSettings->PoolSettings;
	NewPoolSettings.WorldContext = GetWorld();
	
	UPoolObject* NewPoolObject = NewObject<UPoolObject>(PoolOwner);
	NewPoolObject->SetupPoolObject(NewPoolSettings);

	return NewPoolObject;
}