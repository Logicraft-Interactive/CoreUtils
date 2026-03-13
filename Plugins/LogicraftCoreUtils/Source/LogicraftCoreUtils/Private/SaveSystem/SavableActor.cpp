// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#include "SaveSystem/SavableActor.h"


void USavableActorStatics::AddMigrateDelegate(TScriptInterface<ISavableActor> Target, FString FromVersion,
	FString ToVersion, FActorMigrateEventSignature Delegate)
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

void ISavableActor::AddMigrateDelegateNative(const FString& FromVersion, const FString& ToVersion,
	const FActorMigrateEventSignature& Delegate)
{
	UClass* Class = Cast<UObject>(this)->GetClass();
	if (!Class)
	{
		return;
	}
		
	if (!MigratesDelegateMap.Find(Class))
	{
		MigratesDelegateMap.Add(Class);
	}
	
	MigratesDelegateMap[Class].Add(FromVersion, [=](auto Actor, auto FromVersion_, auto OldPropertyArray)
	{
		Delegate.ExecuteIfBound(Actor, FromVersion_, ToVersion, OldPropertyArray);
		return ToVersion;
	});
}



const ISavableActor::FDelegateMapType& ISavableActor::GetMigrateDelegateMap(UObject* This)
{
	if (MigratesDelegateMap.Find(This->GetClass()))
	{
		return MigratesDelegateMap[This->GetClass()];
	}
	return EmptyMap;
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
