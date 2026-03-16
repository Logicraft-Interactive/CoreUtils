// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SavableActor.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/Interface.h"
#include "SavableObject.generated.h"

/**
 * @brief Dynamic delegate signature for component/UObject version migration callbacks.
 *
 * Called during deserialization when a component's saved version does not match
 * the current version. Receives the object being migrated, the source and target
 * version strings, and the old property array for remapping.
 */
DECLARE_DYNAMIC_DELEGATE_FourParams(FObjectMigrateEventSignature, UObject*, Object, FString, FromVersion, FString, ToVersion, const TArray<FPropertySaveData>&, OldPropertyArray);

// UInterface counterpart required by Unreal's reflection system. Do not modify.
UINTERFACE()
class USavableObject : public UInterface
{
	GENERATED_BODY()
};

/**
 * ISavableObject
 *
 * Interface for components and UObjects that participate in the Save System.
 * This is the component-level counterpart of ISavableActor: any UActorComponent
 * implementing this interface will have its SaveGame properties serialized alongside
 * its owning actor, with independent versioning and migration support.
 *
 * Version migration works the same way as ISavableActor: a per-class static delegate
 * map is populated via SetupSaveMigrateLogic_Implementation() on the CDO.
 *
 * @warning BlueprintNativeEvent functions on UInterfaces have a known UHT thunk issue
 *          with multiple inheritance. Use SaveSystemUtils helpers (in SaveSubsystem.cpp)
 *          instead of calling Execute_GetVersion / Execute_SetupSaveMigrateLogic directly.
 */
class LOGICRAFTCOREUTILS_API ISavableObject
{
	GENERATED_BODY()

public:
	/** Map type associating a source version string to a migration function that returns the resulting version. */
	using FDelegateMapType = TMap<FString, TFunction<FString(UObject*, FString, const TArray<FPropertySaveData>&)>>;

protected:
	/** Per-class static map holding version migration delegates. Keyed by UClass*, each entry maps a source version to its migration function. */
	inline static TMap<UClass*, FDelegateMapType> MigratesDelegateMap{};

	/** Empty map returned by GetMigrateDelegateMap when no delegates are registered for a class. */
	inline static FDelegateMapType EmptyMap{};

public:
	
	/**
	 * @brief Returns the current save version string for this component.
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
	void AddMigrateDelegateNative(const FString& FromVersion, const FString& ToVersion, const FObjectMigrateEventSignature& Delegate);

	/**
	 * @brief Registers a C++ lambda as a migration delegate for a specific version transition.
	 *
	 * Delegates are stored in a static per-class map (keyed by UClass*) and apply
	 * to all instances of the class. Typically called from SetupSaveMigrateLogic_Implementation().
	 *
	 * @tparam Func Callable type accepting (FString FromVersion, FString ToVersion, const TArray<FPropertySaveData>&).
	 * @param FromVersion Source version that triggers this migration step.
	 * @param ToVersion Target version after migration.
	 * @param Lambda The migration logic to execute.
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
	
	/** @brief Called on the component before deserialization begins. */
	virtual void OnPreLoad();

	/** @brief Called on the component before serialization begins. */
	virtual void OnPreSave();
	
	/** @brief Called on the component after deserialization and property assignment is complete. */
	virtual void OnPostLoad();

	/** @brief Called on the component after serialization is complete. */
	virtual void OnPostSave();
};

/**
 * USavableUObjectStatics
 *
 * Blueprint function library exposing save system utilities for ISavableObject.
 */
UCLASS()
class LOGICRAFTCOREUTILS_API USavableUObjectStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * @brief Registers a migration delegate on the target component from Blueprint.
	 * @param Target The savable component to register the delegate on.
	 * @param FromVersion Source version string.
	 * @param ToVersion Target version string.
	 * @param Delegate The migration callback to execute.
	 */
	UFUNCTION(BlueprintCallable, Category = "Save System", meta = (DefaultToSelf = "Target"))
	static void AddMigrateDelegate(TScriptInterface<ISavableObject> Target, FString FromVersion, FString ToVersion, FObjectMigrateEventSignature Delegate);
};
