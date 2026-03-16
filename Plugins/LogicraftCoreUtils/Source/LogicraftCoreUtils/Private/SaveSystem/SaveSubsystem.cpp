// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#include "SaveSystem/SaveSubsystem.h"

#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Internationalization/Regex.h"
#include "LogCategory.h"
#include "Kismet/GameplayStatics.h"
#include "SaveSystem/SaveComponent.h"
#include "SaveSystem/SaveableComponent.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "UObject/UObjectIterator.h"

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

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		USaveComponent* SaveComp = It->FindComponentByClass<USaveComponent>();
		if (!SaveComp)
		{
			continue;
		}

		if (auto ActorData = FSaveSerializer::SerializeActor(SaveComp))
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
		MajorVersion = FCString::Atoi(*Matcher.GetCaptureGroup(1));
		MinorVersion = FCString::Atoi(*Matcher.GetCaptureGroup(2));
		PatchVersion = FCString::Atoi(*Matcher.GetCaptureGroup(3));

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
		USaveComponent* SaveComp = It->FindComponentByClass<USaveComponent>();
		if (!SaveComp)
		{
			continue;
		}

		if (!SaveComp->GetIsDynamicSpawned())
		{
			ActorMap.Add(SaveComp->GetUniqueID(), *It);
		}
	}
	return ActorMap;
}

void USaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	for (TObjectIterator<UClass> ClassIterator; ClassIterator; ++ClassIterator)
	{
		UClass* CurrentClass = *ClassIterator;
        
		if (CurrentClass && CurrentClass->IsChildOf(USaveComponent::StaticClass()) && !CurrentClass->HasAnyClassFlags(CLASS_Abstract))
		{
			if (USaveComponent* CDO = CurrentClass->GetDefaultObject<USaveComponent>())
			{
				CDO->SetupSaveMigrateLogic();
			}
		}
		else if (CurrentClass && CurrentClass->IsChildOf(USaveableComponent::StaticClass()) && !CurrentClass->HasAnyClassFlags(CLASS_Abstract))
		{
			if (USaveableComponent* CDO = CurrentClass->GetDefaultObject<USaveableComponent>())
			{
				CDO->SetupSaveMigrateLogic();
			}
		}
	}
}


TOptional<FObjectSaveData> FSaveSerializer::SerializeActor(USaveComponent* SaveComp)
{
	if (!IsValid(SaveComp))
	{
		return NullOpt; 
	}

	AActor* Actor = SaveComp->GetOwner();
	if (!IsValid(Actor))
	{
		return NullOpt;
	}

	SaveComp->OnPreSave();
	
	FObjectSaveData SaveData;
	SaveData.bIsDynamicSpawned = SaveComp->GetIsDynamicSpawned();
	SaveData.ObjectClass = SaveData.bIsDynamicSpawned
		? TSubclassOf<UObject>(SaveComp->GetDynamicSpawnClass().Get())
		: TSubclassOf<UObject>(Actor->GetClass());
	SaveData.ObjectID = SaveComp->GetUniqueID();
	SaveData.SaveVersion = SaveComp->GetSaveVersion();
	SaveData.SpawnTransform = Actor->GetTransform();
	
	UE_LOG(LogSaveSystem, Log, TEXT("Serializing %s (ID: %s, Version: %s)"), 
		   *SaveData.ObjectClass->GetName(), *SaveData.ObjectID, *SaveData.SaveVersion);

	//--------------------------------Saving Actor Properties------------------------------------------ 

    for (TFieldIterator<FProperty> PropIt(Actor->GetClass()); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;

    	if (!Property->HasAnyPropertyFlags(CPF_SaveGame))
    	{
    		continue;
    	}

    	UE_LOG(LogSaveSystem, Verbose, TEXT("  - Found SaveGame property: %s"), *Property->GetName());

		if (Property->IsA<FObjectProperty>())
		{
			UE_LOG(LogSaveSystem, Error, TEXT("UObject reference with SaveGame flag is not supported! (in %s, property %s)"),
				*Actor->GetClass()->GetName(), *Property->GetName())
			continue;
		}
    	
    	FPropertySaveData PropertySaveData;
    	PropertySaveData.PropertyName = Property->GetFName();
    	PropertySaveData.PropertyType = GetPropertyTypeString(Property);
    	
    	void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Actor);
        
    	SerializeProperty(Property, ValuePtr, PropertySaveData);
        
    	SaveData.Properties.Add(MoveTemp(PropertySaveData));
    	++SaveData.PropertiesCount;
	}

	//--------------------------------Saving Components------------------------------------------ 

	TArray<USaveableComponent*> SaveableComponents;
	Actor->GetComponents<USaveableComponent>(SaveableComponents);

	for (USaveableComponent* Component : SaveableComponents)
	{
		Component->OnPreSave();
		
    	UE_LOG(LogSaveSystem, Verbose, TEXT("  - Found Saveable Component: %s"), *Component->GetName());
		FComponentSaveData ComponentSaveData;

		ComponentSaveData.SaveVersion = Component->GetSaveVersion();
		ComponentSaveData.ComponentID = Component->GetName();
		
		for (TFieldIterator<FProperty> PropIt(Component->GetClass()); PropIt; ++PropIt)
		{
			FProperty* Property = *PropIt;

			if (!Property->HasAnyPropertyFlags(CPF_SaveGame))
			{
				continue;
			}

			UE_LOG(LogSaveSystem, Verbose, TEXT("    - Found SaveGame property: %s"), *Property->GetName());
			
			FPropertySaveData PropertySaveData;
			PropertySaveData.PropertyName = Property->GetFName();
			PropertySaveData.PropertyType = GetPropertyTypeString(Property);
    	
			void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Component);
        
			SerializeProperty(Property, ValuePtr, PropertySaveData);
        
			ComponentSaveData.Properties.Add(MoveTemp(PropertySaveData));
			++ComponentSaveData.PropertiesCount;			
		}
		Component->OnPostSave();
		SaveData.Components.Add(MoveTemp(ComponentSaveData));
		++SaveData.ComponentsCount;
	}
	SaveComp->OnPostSave();
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
		
		USaveComponent* SaveComp = Actor->FindComponentByClass<USaveComponent>();
		if (!SaveComp)
		{
			UE_LOG(LogSaveSystem, Error, TEXT("Spawned actor %s has no USaveComponent"), *ObjectSaveData.ObjectID);
			return;
		}
		
		FGuid ActorGuid;
		FGuid::Parse(ObjectSaveData.ObjectID, ActorGuid);
		SaveComp->SetIsDynamicSpawned(TSubclassOf<AActor>(ObjectSaveData.ObjectClass.Get()), ActorGuid);
	}
	else
	{
		AActor* const* FoundActor = StaticSpawnedActorMap.Find(ObjectSaveData.ObjectID);
		if (!FoundActor || !*FoundActor)
		{
			UE_LOG(LogSaveSystem, Error, TEXT("Actor with id %s not found"), *ObjectSaveData.ObjectID);
			return;
		}
		Actor = *FoundActor;
	}

	USaveComponent* SaveComp = Actor->FindComponentByClass<USaveComponent>();
	if (!SaveComp)
	{
		UE_LOG(LogSaveSystem, Error, TEXT("Actor %s has no USaveComponent, skipping load"), *ObjectSaveData.ObjectID);
		return;
	}
	
	SaveComp->OnPreLoad();

	//--------------------------------Migrating logic------------------------------------------

	FString FinalVersion = SaveComp->GetSaveVersion();
	if (FinalVersion != ObjectSaveData.SaveVersion)
	{
		FString CurrentVersion = ObjectSaveData.SaveVersion;
		auto MigrateMap = USaveComponent::GetMigrateDelegateMap(SaveComp);
		while (CurrentVersion != FinalVersion)
		{
			if (!MigrateMap.Contains(CurrentVersion))
			{
				UE_LOG(LogSaveSystem, Error, TEXT("Missing migrating logic for the version %s of the object %s!"
									  " Can't continue the loading of this object"), *CurrentVersion, *Actor->GetClass()->GetName());
				return;
			}
			CurrentVersion = MigrateMap[CurrentVersion](Actor, CurrentVersion, ObjectSaveData.Properties);
		}
	}

	
	//--------------------------------Property loading------------------------------------------ 
	if (ObjectSaveData.Properties.Num() != ObjectSaveData.PropertiesCount)
	{
		UE_LOG(LogSaveSystem, Warning, TEXT("PropertiesCount and Properties.Num() doesn't have the same size, "
									  "possible data corruption in the object %s (class : %s)"),
			*ObjectSaveData.ObjectID, *Actor->GetClass()->GetName());
	}
 
	for (const auto& SaveProp : ObjectSaveData.Properties)
	{
		AssignProperty(Actor, SaveProp);
	}

	//--------------------------------Component loading------------------------------------------ 
	TMap<FString, USaveableComponent*> ComponentsMap;
	TArray<USaveableComponent*> SaveableComponents;
	Actor->GetComponents<USaveableComponent>(SaveableComponents);
	for (USaveableComponent* Component : SaveableComponents)
	{
		ComponentsMap.Add(Component->GetName(), Component);
	}

	if (ObjectSaveData.Components.Num() != ObjectSaveData.ComponentsCount)
	{
		UE_LOG(LogSaveSystem, Warning, TEXT("ComponentsCount and Components.Num() doesn't have the same size, "
									  "possible data corruption in the object %s (class : %s)"),
			*ObjectSaveData.ObjectID, *Actor->GetClass()->GetName());
	}

	for (const auto& ComponentSaveData : ObjectSaveData.Components)
	{
		USaveableComponent** FoundComp = ComponentsMap.Find(ComponentSaveData.ComponentID);
		if (!FoundComp || !*FoundComp)
		{
			continue;
		}
		USaveableComponent* Component = *FoundComp;
		Component->OnPreLoad();
	
		FString CompFinalVersion = Component->GetSaveVersion();
		if (CompFinalVersion != ComponentSaveData.SaveVersion)
		{
			FString CurrentVersion = ComponentSaveData.SaveVersion;
			auto MigrateMap = USaveableComponent::GetMigrateDelegateMap(Component);
			while (CurrentVersion != CompFinalVersion)
			{
				if (!MigrateMap.Contains(CurrentVersion))
				{
					UE_LOG(LogSaveSystem, Error, TEXT("Missing migrating logic for the version %s of the component %s!"
										  " Can't continue the loading of this component"), *CurrentVersion, *Component->GetClass()->GetName());
					break;
				}
				CurrentVersion = MigrateMap[CurrentVersion](Component, CurrentVersion, ComponentSaveData.Properties);
			}
		}
		
		for (const auto& SaveProp : ComponentSaveData.Properties)
		{
			AssignProperty(Component, SaveProp);
		}
		Component->OnPostLoad();
	}

	SaveComp->OnPostLoad();
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
		return;
	}
        
	void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Object);
        

	DeserializeProperty(Property, ValuePtr, SaveProp);
}
