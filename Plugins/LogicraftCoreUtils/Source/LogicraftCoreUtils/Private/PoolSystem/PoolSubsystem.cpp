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
