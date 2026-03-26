// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SaveData.h"
#include "Components/ActorComponent.h"
#include "SaveComponent.generated.h"

/**
 * @brief Dynamic delegate signature for actor version migration callbacks.
 *
 * Called during deserialization when the saved version does not match
 * the current version. The delegate receives the actor being migrated,
 * the source and target version strings, and the old property array
 * so migration logic can remap or default values as needed.
 */
DECLARE_DYNAMIC_DELEGATE_FourParams(FComponentMigrateEventSignature, AActor*, Actor, FString, FromVersion, FString, ToVersion, const TArray<FPropertySaveData>&, OldPropertyArray);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSaveSystemTriggerSignature);

/**
 * USaveComponent
 *
 * Actor component that marks its owning actor as saveable.
 * Attach this component to any actor to include it in the Save System.
 *
 * Handles actor identity (static vs dynamic spawn), versioning,
 * migration delegate registration, and serialization lifecycle callbacks.
 * All functions are BlueprintNativeEvent or BlueprintCallable, making
 * this component 100% usable from Blueprints.
 *
 * The serializer saves the owning actor's UPROPERTY(SaveGame) properties,
 * not the properties of this component itself.
 *
 * Version migration uses a per-class static delegate map:
 * override SetupSaveMigrateLogic in a subclass (C++ or BP) and call
 * AddMigrateDelegate / AddMigrateDelegateLambda to register migration steps.
 */
UCLASS(ClassGroup=(SaveSystem), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName = "Save Component")
class LOGICRAFTCOREUTILS_API USaveComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	using FDelegateMapType = TMap<FString, TFunction<FString(AActor*, FString, const TArray<FPropertySaveData>&)>>;

	USaveComponent();

	/**
	 * @brief Marks the owning actor as dynamically spawned so it will be re-created on load.
	 * @param SpawnClass The class to use for re-spawning.
	 * @param UID Optional unique identifier; a new FGuid is generated if not provided.
	 */
	UFUNCTION(BlueprintCallable, Category = "Save System")
	void SetIsDynamicSpawned(TSubclassOf<AActor> SpawnClass, FGuid UID);

	/** @brief Returns the unique identifier of the owning actor. */
	UFUNCTION(BlueprintPure, Category = "Save System")
	FString GetSaveUniqueID() const;

	/** @brief Returns the class used to re-spawn the owning actor if it is dynamic. */
	UFUNCTION(BlueprintPure, Category = "Save System")
	TSubclassOf<AActor> GetDynamicSpawnClass() const;

	/** @brief Returns true if the owning actor was spawned at runtime. */
	UFUNCTION(BlueprintPure, Category = "Save System")
	bool GetIsDynamicSpawned() const;

	/**
	 * @brief Returns the current save version string.
	 * Override in subclass (C++ or BP) to provide the version.
	 * Compared against the saved version on load to trigger migration.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Save System", DisplayName = "Get Save Version")
	FString GetSaveVersion();

	/**
	* @brief Registers a Blueprint migration delegate for a specific version transition.\n
	 * /!\ This function must be called inside the SetupSaveMigrateLogic function of the interface ISaveableActor.\n
	 * This can result in an undefined behavior if you call it somewhere else.
	 * @param FromVersion Source version that triggers this migration step.
	 * @param ToVersion Target version after migration.
	* @param Delegate Dynamic delegate to execute.
	 */
	UFUNCTION(BlueprintCallable, Category = "Save System")
	void AddMigrateDelegate(const FString& FromVersion, const FString& ToVersion, FComponentMigrateEventSignature Delegate);

	/**
	 * @brief Registers a C++ lambda as a migration delegate for a specific version transition.
	 *
	 * Delegates are stored in a static per-class map (keyed by UClass*) and apply
	 * to all instances of the class. \n
	 * /!\ This function must be called inside the SetupSaveMigrateLogic function of the interface ISaveableActor.\n
	 * This can result in an undefined behavior if you call it somewhere else.
	 *
	 * @tparam Func Callable accepting (AActor*, FString FromVersion, FString ToVersion, const TArray<FPropertySaveData>&).
	 * @param FromVersion Source version that triggers this migration step.
	 * @param ToVersion Target version after migration.
	 * @param Lambda The migration logic to execute.
	 */
	template<typename Func>
	void AddMigrateDelegateLambda(const FString& FromVersion, const FString& ToVersion, Func&& Lambda)
	{
		if (!GetOwner())
		{
			return;
		}
	
		UClass* Class = GetOwner()->GetClass();
		if (!Class)
		{
			return;
		}
		
		if (!MigratesDelegateMap.Find(Class))
		{
			MigratesDelegateMap.Add(Class);
		}
		
		MigratesDelegateMap[Class].Add(FromVersion, [MigrateLogic = Forward<Func>(Lambda), ToVersion]
			(AActor* Actor, FString From, const TArray<FPropertySaveData>& PropertyArray)
		{
			MigrateLogic(Actor, From, ToVersion, PropertyArray);
			return ToVersion;
		});
	}

	/** @brief Returns the migration delegate map for the given component's class. */
	static FDelegateMapType& GetMigrateDelegateMap(const UObject* Object);
	
	
	static TMap<UClass*, FDelegateMapType>& GetAllMigrateDelegateMap();

	// ---- Serialization lifecycle callbacks ----

	UPROPERTY(BlueprintAssignable, Category = "Save System")
	FOnSaveSystemTriggerSignature OnPreSave;

	UPROPERTY(BlueprintAssignable, Category = "Save System")
	FOnSaveSystemTriggerSignature OnPostSave;
	
	UPROPERTY(BlueprintAssignable, Category = "Save System")
	FOnSaveSystemTriggerSignature OnPreLoad;

	UPROPERTY(BlueprintAssignable, Category = "Save System")
	FOnSaveSystemTriggerSignature OnPostLoad;

protected:
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save System")
	bool bIsDynamicSpawned = false;

	UPROPERTY()
	TSubclassOf<AActor> DynamicSpawnClass = nullptr;

	UPROPERTY()
	FString UniqueID;

	inline static TMap<UClass*, FDelegateMapType> MigratesDelegateMap{};
	inline static FDelegateMapType EmptyMap{};
};
