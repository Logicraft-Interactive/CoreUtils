// Copyright (c) Logicraft Interactive. All Rights Reserved.


#include "SaveSystem/SavableActor.h"


void USavableActorStatics::AddMigrateDelegate(TScriptInterface<ISavableActor> Target, FString FromVersion,
	FString ToVersion, FMigrateEventSignature Delegate)
{
	if (Target.GetInterface())
	{
		Target->AddMigrateDelegateNative(FromVersion, ToVersion, Delegate);
	}
}

void ISavableActor::SetIsDynamicSpawned(TSubclassOf<UObject> SpawnActor, FGuid UID)
{
	DynamicSpawnClass = SpawnActor;
	UniqueID = UID.ToString();
	bIsDynamicSpawned = true;
}

FString ISavableActor::GetUniqueID() const
{
	return bIsDynamicSpawned ? UniqueID : _getUObject()->GetName();
}

TSubclassOf<UObject> ISavableActor::GetDynamicSpawnClass() const
{
	return DynamicSpawnClass;
}

bool ISavableActor::GetIsDynamicSpawned() const
{
	return bIsDynamicSpawned;
}

FString ISavableActor::BP_GetVersion_Implementation()
{
	return GetVersion();
}

void ISavableActor::AddMigrateDelegateNative(const FString& FromVersion, const FString& ToVersion,
	const FMigrateEventSignature& Delegate)
{
	MigratesDelegateMap.Add(FromVersion, [=](auto FromVersion_, auto OldPropertyArray)
	{
		Delegate.ExecuteIfBound(FromVersion_, ToVersion, OldPropertyArray);
		return ToVersion;
	});
}


const ISavableActor::DelegateMapType& ISavableActor::GetMigrateDelegateMap()
{
	return MigratesDelegateMap;
}

// Add default functionality here for any ISavableActor functions that are not pure virtual.
void ISavableActor::OnPreLoad()
{
}

void ISavableActor::OnPreSave()
{
}

void ISavableActor::OnPostLoad()
{
}

void ISavableActor::OnPostSave()
{
}
