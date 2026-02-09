// Copyright (c) Logicraft Interactive. All Rights Reserved.


#include "SaveSystem/SavableActor.h"


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

void ISavableActor::AddMigrateVersionLogic(const FString& Version,
	const TFunction<FPropertyTuple(int, int, int, FPropertyTuple)>& MigrateLogic)
{
	MigrateLogics.Add(Version, MigrateLogic);
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
