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
	const FPoolSettings NewPoolSettings{.SpawnClass = SpawnClass,
	.WorldContext = GetWorld(),
	.bAllowResize = PoolSettings->bAllowResize,
	.bAutoShrink = PoolSettings->bAutoShrink,
	.MinPoolSize = PoolSettings->MinPoolSize,
	.AutoShrinkUpdateTime = PoolSettings->AutoShrinkUpdateTime,
	.ShrinkObjectAfterReturnTime = PoolSettings->ShrinkObjectAfterReturnTime};

	UPoolObject* NewPoolObject = NewObject<UPoolObject>(this);
	NewPoolObject->SetupPoolObject(NewPoolSettings);

	return NewPoolObject;
}
