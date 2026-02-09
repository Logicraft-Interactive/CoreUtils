// Copyright (c) Logicraft Interactive. All Rights Reserved.


#include "SaveSystem/SavableActor.h"


void ISavableActor::SetIsDynamicSpawned(TSubclassOf<AActor> SpawnActor, FGuid UID)
{
	DynamicSpawnClass = SpawnActor;
	UniqueID = *UID.ToString();
	bIsDynamicSpawned = true;
}

FName ISavableActor::GetUniqueID() const
{
	return UniqueID;
}

TSubclassOf<AActor> ISavableActor::GetDynamicSpawnClass() const
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

void ISavableActor::Migrate(int MajorVersion, int MinorVersion, int PatchVersion, FPropertyTuple Properties)
{
	
	
	FPropertyTuple NewProperties = 
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
