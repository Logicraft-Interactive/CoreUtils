// Copyright (c) Logicraft Interactive. All Rights Reserved.


#include "SaveSystem/SaveSubsystem.h"

#include "EngineUtils.h"
#include "LogCategory.h"
#include "Kismet/GameplayStatics.h"
#include "SaveSystem/SavableActor.h"
#include "SaveSystem/SavableObject.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

void USaveSubsystem::SaveWorld(const FName& SlotName, const FString& Version)
{
	TOptional<VersionType> OptionalVersionTuple = ExtractVersion(Version);

	if (!OptionalVersionTuple.IsSet())
	{
		UE_LOG(LogSaveSystem, Error, TEXT("Can't continue saving with invalid version format : %s"), *Version);
		return;
	}

	if (!CurrentSave)
	{
		CurrentSave = Cast<ULCUSaveGame>(UGameplayStatics::CreateSaveGameObject(ULCUSaveGame::StaticClass()));
	}
	
	CurrentSave->ObjectsData.Empty();
	CurrentSave->SaveSlotName = SlotName;
	CurrentSave->SaveTimeStamp = FDateTime::Now();
	CurrentSave->GlobalSaveVersion = Version;

	TArray<AActor*> SavableActors;
	UGameplayStatics::GetAllActorsWithInterface(GetWorld(), USavableActor::StaticClass(), SavableActors);

	for (auto SavableActor : SavableActors)
	{
		if (auto ActorData = FSaveSerializer::SerializeActor(Cast<ISavableActor>(SavableActor), Version))
		{
			CurrentSave->ObjectsData.Add(MoveTemp(*ActorData));
		}		
	}

	UGameplayStatics::SaveGameToSlot(CurrentSave, SlotName.ToString(), 0);
}

void USaveSubsystem::LoadWorld(const FName& SlotName, const FString& Version)
{
	TOptional<VersionType> OptionalVersionTuple = ExtractVersion(Version);

	if (!OptionalVersionTuple.IsSet())
	{
		UE_LOG(LogSaveSystem, Error, TEXT("Can't continue loading with invalid version format : %s"), *Version);
		return;
	}
	
	CurrentSave = Cast<ULCUSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName.ToString(), 0));
	if (!CurrentSave)
	{
		UE_LOG(LogSaveSystem, Error, TEXT("Can't load at the slot : %s"), *SlotName.ToString());
		return;
	}

	
	auto ActorCache = BuildStaticActorSpawnedMap();

	for (auto ObjectData : CurrentSave->ObjectsData)
	{
		FSaveSerializer::DeserializeActor(this, ObjectData, Version, ActorCache);
	}
}

TOptional<USaveSubsystem::VersionType> USaveSubsystem::ExtractVersion(const FString& Version)
{
	int MajorVersion{1}, MinorVersion{0}, PatchVersion{0};

	const FRegexPattern Pattern(TEXT("^(\\d+)\\.(\\d+)\\.(\\d+)$"));
    
	FRegexMatcher Matcher(Pattern, Version);

	if (Matcher.FindNext())
	{
		// Extract groups. Index 1 is the first group, 2 is the second, etc.
		// Convert the captured string to an integer
		MajorVersion = FCString::Atoi(*Matcher.GetCaptureGroup(1));
		MinorVersion = FCString::Atoi(*Matcher.GetCaptureGroup(2));
		PatchVersion = FCString::Atoi(*Matcher.GetCaptureGroup(3));

		// Log for debugging
		UE_LOG(LogSaveSystem, Log, TEXT("Parsed Version: %d.%d.%d"), MajorVersion, MinorVersion, PatchVersion);
	}
	else
	{
		UE_LOG(LogSaveSystem, Warning, TEXT("Invalid version format: %s"), *Version);
		return NullOpt;
	}

	return VersionType(MajorVersion, MinorVersion, PatchVersion);
}

TMap<FString, AActor*> USaveSubsystem::BuildStaticActorSpawnedMap() const
{
	TMap<FString, AActor*> ActorMap;

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (!It->Implements<USavableActor>())
		{
			continue;
		}

		ISavableActor* Object = Cast<ISavableActor>(*It);
		if (!Object->GetIsDynamicSpawned())
		{
			ActorMap.Add(Object->GetUniqueID(), *It);
		}
	}
	return ActorMap;
}


TOptional<FObjectSaveData> FSaveSerializer::SerializeActor(ISavableActor* SavableActor, FString Version)
{
	if (!IsValid(SavableActor->_getUObject()))
	{
		return NullOpt; 
	}

	auto Actor = Cast<AActor>(SavableActor->_getUObject());
	if (!Actor)
	{
		return NullOpt;
	}
	//--------------------------------Init Actor------------------------------------------ 
	SavableActor->OnPreSave();
	
	FObjectSaveData SaveData;
	SaveData.bIsDynamicSpawned = SavableActor->GetIsDynamicSpawned();
	SaveData.ObjectClass = SaveData.bIsDynamicSpawned ? SavableActor->GetDynamicSpawnClass() : TSubclassOf<UObject>(SavableActor->_getUObject()->GetClass());
	SaveData.ObjectID = SavableActor->GetUniqueID();
	SaveData.SaveVersion = SavableActor->GetVersion();
	SaveData.SpawnTransform = Actor->GetTransform();
	
	UE_LOG(LogSaveSystem, Log, TEXT("Serializing %s (ID: %s, Version: %s)"), 
		   *SaveData.ObjectClass->GetName(), *SaveData.ObjectID, *SaveData.SaveVersion);

	//--------------------------------Saving Property------------------------------------------ 

    for (TFieldIterator<FProperty> PropIt(SaveData.ObjectClass); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;

    	if (!Property->HasAnyPropertyFlags(CPF_SaveGame))
    	{
    		continue;
    	}

    	UE_LOG(LogSaveSystem, Verbose, TEXT("  - Found SaveGame property: %s"), *Property->GetName());

		if (Property->IsA<FObjectProperty>())
		{
			
			UE_LOG(LogSaveSystem, Error, TEXT("UObject was found with the property SaveGame! use the adequate interface (ISavableObject)! (in %s, property %s)"),
				*SaveData.ObjectClass->GetName(), *Property->GetName())
			continue;
		}
    	
    	FPropertySaveData PropertySaveData;
    	PropertySaveData.PropertyName = Property->GetFName();
    	PropertySaveData.PropertyType = GetPropertyTypeString(Property);
    	
    	void* ValuePtr = Property->ContainerPtrToValuePtr<void>(SavableActor->_getUObject());
        
    	SerializeProperty(Property, ValuePtr, PropertySaveData);
        
    	SaveData.Properties.Add(MoveTemp(PropertySaveData));
    	++SaveData.PropertiesCount;
	}

	//--------------------------------Saving Components------------------------------------------ 

	for (auto Component : Actor->GetComponentsByInterface(USavableObject::StaticClass()))
	{
		ISavableObject* SavableComponent = Cast<ISavableObject>(Component);
		SavableComponent->OnPreSave();
		
    	UE_LOG(LogSaveSystem, Verbose, TEXT("  - Found Savable Component: %s"), *Component->GetName());
		FComponentSaveData ComponentSaveData;

		ComponentSaveData.SaveVersion = Version;
		ComponentSaveData.ComponentID = Component->GetName();
		
		for (TFieldIterator<FProperty> PropIt(Component->GetClass()); PropIt; ++PropIt)
		{
			FProperty* Property = *PropIt;

			if (!Property->HasAnyPropertyFlags(CPF_SaveGame))
			{
				continue;
			}

			UE_LOG(LogSaveSystem, Verbose, TEXT("  - Found SaveGame property: %s"), *Property->GetName());
			
			FPropertySaveData PropertySaveData;
			PropertySaveData.PropertyName = Property->GetFName();
			PropertySaveData.PropertyType = GetPropertyTypeString(Property);
    	
			void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Component);
        
			SerializeProperty(Property, ValuePtr, PropertySaveData);
        
			ComponentSaveData.Properties.Add(MoveTemp(PropertySaveData));
			++ComponentSaveData.PropertiesCount;			
		}
		SavableComponent->OnPostSave();
		SaveData.Components.Add(MoveTemp(ComponentSaveData));
		++SaveData.ComponentsCount;
	}
	SavableActor->OnPostSave();
	return SaveData;
}

void FSaveSerializer::DeserializeActor(UObject* WorldContext, const FObjectSaveData& ObjectSaveData, FString Version,
	const TMap<FString, AActor*>& StaticSpawnedActorMap)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return;
	}

	ISavableActor* SavableActor = nullptr;
	AActor* Actor = nullptr;
	
	//--------------------------------Init Actor------------------------------------------ 
	if (ObjectSaveData.bIsDynamicSpawned)
	{
		if (!ObjectSaveData.ObjectClass)
		{
			UE_LOG(LogSaveSystem, Error, TEXT("No spawn class found for %s object"), *ObjectSaveData.ObjectID);
			return;
		}
		
		Actor = World->SpawnActor(ObjectSaveData.ObjectClass);
		Actor->SetActorTransform(ObjectSaveData.SpawnTransform);
		SavableActor = Cast<ISavableActor>(Actor);
		
		FGuid ActorGuid;
		FGuid::Parse(ObjectSaveData.ObjectID,ActorGuid);
		SavableActor->SetIsDynamicSpawned(ObjectSaveData.ObjectClass, ActorGuid);
	}
	else
	{
		Actor = StaticSpawnedActorMap[ObjectSaveData.ObjectID];
		if (!Actor)
		{
			UE_LOG(LogSaveSystem, Error, TEXT("Actor with id %s not found"), *ObjectSaveData.ObjectID);
			return;
		}
		SavableActor = Cast<ISavableActor>(Actor);
	}
	SavableActor->OnPreLoad();

	//--------------------------------Migrating logic------------------------------------------

	if (FString FinalVersion = ISavableActor::Execute_BP_GetVersion(SavableActor->_getUObject()); FinalVersion != ObjectSaveData.SaveVersion)
	{
		FString CurrentVersion = ObjectSaveData.SaveVersion;
		auto MigrateMap = SavableActor->GetMigrateDelegateMap();
		while (CurrentVersion != FinalVersion)
		{
			if (!MigrateMap.Contains(CurrentVersion))
			{
				UE_LOG(LogSaveSystem, Error, TEXT("Missing migrating logic for the version %s of the object %s!"
									  " Can't continue the loading of this object"), *CurrentVersion, *SavableActor->_getUObject()->GetClass()->GetName());
				return;
			}
			CurrentVersion = MigrateMap[CurrentVersion](CurrentVersion, ObjectSaveData.Properties);
		}
	}

	
	//--------------------------------Property loading------------------------------------------ 
	if (ObjectSaveData.Properties.Num() != ObjectSaveData.PropertiesCount)
	{
		UE_LOG(LogSaveSystem, Warning, TEXT("PropertiesCount and Properties.Num() doesn't have the same size, "
									  "possible data corruption in the object %s (class : %s)"),
			*ObjectSaveData.ObjectID, *Actor->GetClass()->GetName());
	}
 
	for (auto SaveProp : ObjectSaveData.Properties)
	{
		AssignProperty(Actor, SaveProp);
	}

	//--------------------------------Component loading------------------------------------------ 
	TMap<FString, UActorComponent*> ComponentsMap;
	for (auto Component : Actor->GetComponentsByInterface(USavableObject::StaticClass()))
	{
		ComponentsMap.Add(Component->GetName(), Component);
	}

	if (ObjectSaveData.Components.Num() != ObjectSaveData.ComponentsCount)
	{
		UE_LOG(LogSaveSystem, Warning, TEXT("ComponentsCount and Components.Num() doesn't have the same size, "
									  "possible data corruption in the object %s (class : %s)"),
			*ObjectSaveData.ObjectID, *Actor->GetClass()->GetName());
	}

	for (auto ComponentSaveData : ObjectSaveData.Components)
	{
		auto Component = ComponentsMap[ComponentSaveData.ComponentID];
		ISavableObject* SavableComponent = Cast<ISavableObject>(Component);
		SavableComponent->OnPreLoad();
		if (!Component)
		{
			continue;
		}

		for (auto SaveProp : ComponentSaveData.Properties)
		{
			AssignProperty(Component, SaveProp);
		}
		SavableComponent->OnPostLoad();
	}

	SavableActor->OnPostLoad();
}




FString FSaveSerializer::GetPropertyTypeString(const FProperty* Property)
{    
	return Property->GetCPPType();
}

void FSaveSerializer::SerializeProperty(FProperty* Property, void* ValuePtr, FPropertySaveData& OutSaveProp)
{
	TArray<uint8> PropertyData;
	FMemoryWriter MemoryWriter(PropertyData);
    

	FObjectAndNameAsStringProxyArchive ProxyArchive(MemoryWriter, false);
	
	FStructuredArchiveFromArchive StructuredArchive(ProxyArchive);
	
	Property->SerializeItem(StructuredArchive.GetSlot(), ValuePtr);
    
	OutSaveProp.PropertyData = PropertyData;
}

void FSaveSerializer::DeserializeProperty(FProperty* Property, void* ValuePtr, const FPropertySaveData& SaveProp)
{
	TArray<uint8> PropertyData = SaveProp.PropertyData;
	FMemoryReader MemoryReader(PropertyData);
    
	FObjectAndNameAsStringProxyArchive ProxyArchive(MemoryReader, true);
	
	FStructuredArchiveFromArchive StructuredArchive(ProxyArchive);
    
	Property->SerializeItem(StructuredArchive.GetSlot(), ValuePtr);
}

void FSaveSerializer::AssignProperty(UObject* Object, const FPropertySaveData& SaveProp)
{
	if (!IsValid(Object))
	{
		return;
	}
		
	const UClass* Class = Object->GetClass();
	FProperty* Property = Class->FindPropertyByName(SaveProp.PropertyName);
        
	if (!Property)
	{
		UE_LOG(LogTemp, Warning, TEXT("Property '%s' not found (removed?)"), 
			   *SaveProp.PropertyName.ToString());
		return;
	}
        
	FString CurrentType = GetPropertyTypeString(Property);
	if (CurrentType != SaveProp.PropertyType)
	{
		UE_LOG(LogTemp, Error, TEXT("Property '%s' type mismatch: saved=%s, current=%s"), 
			   *SaveProp.PropertyName.ToString(), *SaveProp.PropertyType, *CurrentType);
		return;;
	}
        
	void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Object);
        

	DeserializeProperty(Property, ValuePtr, SaveProp);
}

