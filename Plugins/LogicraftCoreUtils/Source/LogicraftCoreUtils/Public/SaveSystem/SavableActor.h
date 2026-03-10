// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SaveData.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/Interface.h"
#include "SavableActor.generated.h"

// UInterface counterpart required by Unreal's reflection system. Do not modify.
UINTERFACE()
class USavableActor : public UInterface
{
	GENERATED_BODY()
};

/**
 * @brief Dynamic delegate signature for actor version migration callbacks.
 *
 * Called during deserialization when the saved version does not match
 * the current version. The delegate receives the actor being migrated,
 * the source and target version strings, and the old property array
 * so migration logic can remap or default values as needed.
 */
DECLARE_DYNAMIC_DELEGATE_FourParams(FActorMigrateEventSignature, AActor*, Actor, FString, FromVersion, FString, ToVersion, const TArray<FPropertySaveData>&, OldPropertyArray);

/**
 * ISavableActor
 *
 * Interface for actors that participate in the Save System.
 * Implementing this interface allows an actor to be serialized, deserialized,
 * and versioned with automatic migration support.
 *
 * Actors can be either **static** (placed in the level, matched by name on load)
 * or **dynamic** (runtime-spawned, re-created from their class on load).
 *
 * Version migration is handled through a per-class static delegate map.
 * Override SetupSaveMigrateLogic_Implementation() on the CDO to register
 * migration steps via AddMigrateDelegateLambda / AddMigrateDelegateNative.
 *
 * @warning BlueprintNativeEvent functions on UInterfaces have a known UHT thunk issue
 *          with multiple inheritance. Use SaveSystemUtils helpers (in SaveSubsystem.cpp)
 *          instead of calling Execute_GetVersion / Execute_SetupSaveMigrateLogic directly.
 */
class LOGICRAFTCOREUTILS_API ISavableActor
{
	GENERATED_BODY()

public:
	/** Map type associating a source version string to a migration function that returns the resulting version. */
	using FDelegateMapType = TMap<FString, TFunction<FString(AActor*, FString, const TArray<FPropertySaveData>&)>>;

protected:
	/** Whether this actor was spawned at runtime (true) or placed in the level (false). */
	bool bIsDynamicSpawned = false;

	/** Class to use when re-spawning this actor during deserialization. Only relevant for dynamic actors. */
	TSubclassOf<UObject> DynamicSpawnClass = nullptr;

	/** Unique identifier for this actor instance (FGuid string for dynamic, actor name for static). */
	FString UniqueID;

	/** Per-class static map holding version migration delegates. Keyed by UClass*, each entry maps a source version to its migration function. */
	inline static TMap<UClass*, FDelegateMapType> MigratesDelegateMap{};

	/** Empty map returned by GetMigrateDelegateMap when no delegates are registered for a class. */
	inline static FDelegateMapType EmptyMap{};
	
public:
	
	/**
	 * @brief Marks this actor as dynamically spawned so it will be re-created on load.
	 * @param SpawnActor The class to use for re-spawning.
	 * @param UID Optional unique identifier; a new FGuid is generated if not provided.
	 */
	void SetIsDynamicSpawned(TSubclassOf<UObject> SpawnActor, FGuid UID = FGuid::NewGuid());

	/** @brief Returns the unique identifier of this actor instance. */
	FString GetUniqueID() const;

	/** @brief Returns the class used to re-spawn this actor if it is dynamic. */
	TSubclassOf<UObject> GetDynamicSpawnClass() const;

	/** @brief Returns true if this actor was spawned at runtime. */
	bool GetIsDynamicSpawned() const;

	/**
	 * @brief Returns the current save version string for this actor.
	 *
	 * Override GetVersion_Implementation() in your class to provide the version.
	 * The version is compared against the saved version on load to trigger migration.
	 */
	UFUNCTION(BlueprintNativeEvent, DisplayName = "Get Save Version")
	FString GetVersion();

	/**
	 * @brief Registers a Blueprint-compatible migration delegate for a specific version transition.
	 * @param FromVersion Source version that triggers this migration step.
	 * @param ToVersion Target version after migration.
	 * @param Delegate Dynamic delegate to execute.
	 */
	void AddMigrateDelegateNative(const FString& FromVersion, const FString& ToVersion,const FActorMigrateEventSignature& Delegate);
	
	/**
	 * @brief Registers a C++ lambda as a migration delegate for a specific version transition.
	 *
	 * Delegates are stored in a static per-class map (keyed by UClass*) and apply
	 * to all instances of the class. Typically called from SetupSaveMigrateLogic_Implementation().
	 *
	 * @tparam Func Callable type accepting (AActor*, FString FromVersion, FString ToVersion, const TArray<FPropertySaveData>&).
	 * @param FromVersion Source version that triggers this migration step.
	 * @param ToVersion Target version after migration.
	 * @param Lambda The migration logic to execute. Signature need to be FActorMigrateEventSignature
	 */
	template<typename Func>
	void AddMigrateDelegateLambda(const FString& FromVersion, const FString& ToVersion, Func&& Lambda)
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
		
		MigratesDelegateMap[Class].Add(FromVersion, [MigrateLogic = Forward<Func>(Lambda), ToVersion](auto Actor, auto From, auto PropertyArray)
		{
			MigrateLogic(Actor, From, ToVersion, PropertyArray);
			return ToVersion;
		});
	}

	/**
	 * @brief Returns the migration delegate map for the given object's class.
	 * @param This The UObject whose class is used to look up registered delegates.
	 * @return Reference to the delegate map, or an empty map if none registered.
	 */
	static const FDelegateMapType& GetMigrateDelegateMap(UObject* This); 

	/**
	 * @brief Entry point for registering version migration delegates.
	 *
	 * Called once on each class CDO during USaveSubsystem::Initialize.
	 * Override SetupSaveMigrateLogic_Implementation() to register migration
	 * steps via AddMigrateDelegateLambda or AddMigrateDelegateNative.
	 */
	UFUNCTION(BlueprintNativeEvent, DisplayName = "Setup Save Migrate Logic")
	void SetupSaveMigrateLogic();

	// ---- Serialization lifecycle callbacks ----
	
	/** @brief Called on the actor before deserialization begins. */
	virtual void OnPreLoad();

	/** @brief Called on the actor before serialization begins. */
	virtual void OnPreSave();
	
	/** @brief Called on the actor after deserialization and property assignment is complete. */
	virtual void OnPostLoad();

	/** @brief Called on the actor after serialization is complete. */
	virtual void OnPostSave();
};

/**
 * USavableActorStatics
 *
 * Blueprint function library exposing save system utilities for ISavableActor.
 */
UCLASS()
class LOGICRAFTCOREUTILS_API USavableActorStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * @brief Registers a migration delegate on the target actor from Blueprint.
	 * @param Target The savable actor to register the delegate on.
	 * @param FromVersion Source version string.
	 * @param ToVersion Target version string.
	 * @param Delegate The migration callback to execute.
	 */
	UFUNCTION(BlueprintCallable, Category = "Save System", meta = (DefaultToSelf = "Target"))
	static void AddMigrateDelegate(TScriptInterface<ISavableActor> Target, FString FromVersion, FString ToVersion, FActorMigrateEventSignature Delegate);
};
