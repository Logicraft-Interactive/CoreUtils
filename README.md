# LogicraftCoreUtils

**An Unreal Engine 5.7 plugin containing utility functions, classes, and structs designed to make Unreal C++ code more readable and simpler.**

Developed by [Logicraft Interactive](https://github.com/Logicraft-Interactive).

---

## Table of Contents

- [Overview](#overview)
- [Requirements](#requirements)
- [Installation](#installation)
- [Modules](#modules)
  - [Chain (Monadic Safe Chaining)](#chain)
  - [EventBus (Tag-Based Event System)](#eventbus)
  - [Linq (Query Operations)](#linq)
  - [TimerHolder (RAII Timer Wrapper)](#timerholder)
  - [Pool System (Object Pooling)](#pool-system)
  - [Save System (Serialization and Persistence)](#save-system)
  - [Runtime Property Editor (In-Game Property Editing)](#runtime-property-editor)
- [Meta Utilities](#meta-utilities)
- [Log Categories](#log-categories)
- [License](#license)

---

## Overview

LogicraftCoreUtils is a C++ plugin for Unreal Engine 5.7 that provides a collection of systems commonly needed in game development. Each module is designed around modern C++20 patterns (concepts, consteval, CRTP) and integrates tightly with Unreal Engine conventions (subsystems, `UPROPERTY`, `UFUNCTION`, Gameplay Tags, Slate).

The plugin ships seven core modules:

| Module | Purpose | Subsystem Type |
|--------|---------|----------------|
| **Chain** | Null-safe method chaining (Maybe Monad) | None (free functions) |
| **EventBus** | Type-safe, thread-safe event bus using Gameplay Tags | `UGameInstanceSubsystem` |
| **Linq** | LINQ-style lazy query operations on containers | None (free functions) |
| **TimerHolder** | RAII timer wrapper with automatic cleanup | None (value type) |
| **Pool System** | Actor object pooling with auto-shrink | `UWorldSubsystem` |
| **Save System** | Actor/component serialization with version migration | `UGameInstanceSubsystem` |
| **Runtime Property Editor** | Slate-based property editor for packaged builds | `UWorldSubsystem` |

---

## Requirements

- Unreal Engine **5.7**
- C++20-compatible compiler

### Module Dependencies

| Dependency | Visibility |
|------------|------------|
| `Core` | Public |
| `GameplayTags` | Public |
| `Slate` | Public |
| `SlateCore` | Public |
| `InputCore` | Public |
| `DeveloperSettings` | Public |
| `CoreUObject` | Private |
| `Engine` | Private |
| `UnrealEd` | Private (Editor only) |

---

## Installation

1. Clone or copy this repository into the `Plugins/` directory of your Unreal Engine project.
2. Regenerate project files.
3. Add `"LogicraftCoreUtils"` to your module's `Build.cs` dependencies:

```csharp
PublicDependencyModuleNames.Add("LogicraftCoreUtils");
```

4. Include the headers you need in your C++ files.

---

## Modules

---

### Chain

> Header: `Chain.h`

A namespace containing utilities for **safe method chaining on UObjects**, implementing a Monadic pattern (Maybe Monad). This pattern allows executing sequences of operations where validity checks (`IsValid`) are handled automatically between steps, significantly reducing nested `if` statements and "arrow code".

#### Core Class: `TChain<T>`

A wrapper class that holds a UObject pointer and manages safe execution flow. If the object is invalid (null or pending kill), subsequent `Execute` / `Transform` calls are skipped and an optional warning is logged. The wrapped object is stored as a `TWeakObjectPtr` to be safe against garbage collection during the chain execution.

#### API

| Method | Description |
|--------|-------------|
| `Execute(Function)` | Invokes a callable (void function) on the wrapped object **only if it is valid**. Supports lambdas, member function pointers, and functors via `std::invoke`. Returns a reference to self for further chaining. |
| `Transform(Function)` | Transforms the chain into a chain of a **different type/object**. Useful for navigating object hierarchies (e.g., `Actor -> GetComponent -> GetChild`). If the current object is invalid, it returns an invalid chain of the new type immediately. |
| `GetValue(Function, DefaultValue)` | Terminates the chain and returns a value derived from the object. Returns `DefaultValue` if the chain is broken. |
| `Cast<NewType>()` | Safely attempts to cast the internal object to a specific UObject subclass. If the cast fails, the chain becomes invalid. |
| `OrDefault(DefaultObject)` | Provides a fallback object if the current chain is invalid. |
| `OrElse(Function)` | Executes a fallback function if the chain is invalid. Useful for error handling at the end of a chain. |
| `Get()` | Explicit accessor to the underlying raw pointer (may be `nullptr`). |

#### Entry Points

| Function | Description |
|----------|-------------|
| `Chain::StartChain(Object, bThrowWarning)` | Starts a new chain from a UObject pointer or reference. |
| `Chain::Execute(Object, Function, bThrowWarning)` | Immediately executes a single function on an object with safety checks, without creating an explicit chain. |
| `Chain::Transform(Object, Function, bThrowWarning)` | Immediately transforms an object with safety checks. Short-hand for `StartChain(Obj).Transform(...)`. |

#### Example

```cpp
// Navigate an object hierarchy safely
Chain::StartChain(MyActor)
    .Execute([](AMyActor* Actor) { Actor->DoSomething(); })
    .Transform([](AMyActor* Actor) { return Actor->GetComponentByClass<UHealthComponent>(); })
    .Execute([](UHealthComponent* Health) { Health->Heal(50.f); })
    .OrElse([]() { UE_LOG(LogTemp, Warning, TEXT("Chain broke!")); });
```

> **Warning**: The `->` and `*` operators on `TChain` do **not** perform validity checks and will crash on null. Prefer using `Execute()` for safe access.

---

### EventBus

> Header: `EventBus.h`

A type-safe, thread-safe event bus that maps **Gameplay Tags** to multicast delegates. It facilitates decoupling by allowing disparate systems to communicate via tags without direct references.

#### Runtime Strict Typing

The first delegate bound to a Gameplay Tag determines the expected signature (argument types) for that tag. All subsequent bindings or broadcasts **must** match this signature. The signature is released once all subscribers are removed, unless explicitly locked via `LockSignature`.

#### Thread-Safety Model

The internal `ActiveEvents` map is protected by an `FRWLock`:

- **Write operations** (`Add`, `Remove`, `LockSignature`, `UnlockSignature`) acquire a write lock for the duration of the map access.
- **Read operations** (`IsBound`, `IsBoundToObject`, `IsArgsType`) acquire a read lock.
- **Broadcast** acquires a short read lock only to extract a `TSharedPtr` to the container, then releases the lock before invoking callbacks. This prevents deadlocks when a callback re-enters the EventBus (e.g., calls `Add` or `Remove`). The `TSharedPtr` keeps the container alive even if another thread removes it from the map between the unlock and the broadcast.

The delegate invocation lists themselves are **not** protected by default. Concurrent `Add`/`Remove` and `Broadcast` on the same tag from different threads is undefined behavior unless `EVENTBUS_THREAD_SAFE_DELEGATES` is enabled.

<details>
<summary><strong>EVENTBUS_THREAD_SAFE_DELEGATES</strong></summary>

When enabled (`#define EVENTBUS_THREAD_SAFE_DELEGATES 1`), each `TEventContainer` uses `FDefaultTSDelegateUserPolicy`, which protects the delegate's internal invocation list with a `FTransactionallySafeCriticalSection` (a true system mutex, not a spinlock). Note that UE's implementation uses the same mutex for both reads and writes, meaning concurrent reads are also mutually exclusive.

**Warning**: Because reads and writes share the same mutex, high-frequency broadcasts or heavy callbacks will serialize all concurrent accesses and may hurt performance. Only enable this if you truly need concurrent `Add`/`Remove` and `Broadcast` on the same tag from different threads.

By default this is disabled: the EventBus `FRWLock` already protects the active event map, which is sufficient for the typical GameThread usage pattern.

</details>

#### Subscription Methods

| Method | Description |
|--------|-------------|
| `AddUObject(Tag, Object, Function)` | Binds a UObject member function. Stores a weak reference; the delegate is automatically skipped if the object has been garbage collected. |
| `AddUFunction(Tag, Object, FunctionName)` | Binds a reflected `UFunction` (callable from Blueprint). Stores a weak reference. |
| `AddDelegate(Tag, Delegate)` | Adds a pre-built `TDelegate` instance to the invocation list. |
| `AddWeakLambda(Tag, Object, Functor)` | Adds a lambda bound to a UObject. The lambda is not invoked if the object has been garbage collected. |
| `AddLambda(Tag, Functor)` | Adds a raw lambda with no lifetime management. Caller is responsible for removing it before it becomes invalid. |
| `AddRaw(Tag, Object, Function)` | Adds a raw C++ pointer member function delegate. No lifetime management. |
| `AddSP(Tag, SharedRef, Function)` | Adds a shared pointer-based member function delegate. Stores a weak reference; skipped if the shared object expires. Automatically selects `AddThreadSafeSP` when `Mode == ESPMode::ThreadSafe`. |
| `AddSPLambda(Tag, Object, Functor)` | Adds a weak shared pointer lambda delegate. Skipped if the shared object has expired. |
| `AddStatic(Tag, Function)` | Adds a static (free) function delegate. Always invoked unconditionally. |

#### Signature Locking

| Method | Description |
|--------|-------------|
| `LockSignature<TArgs...>(Tag)` | Pre-associates a specific delegate signature with the tag and prevents the container from being removed when its subscriber count reaches zero. Useful to reserve a tag's signature before any subscriber registers, or to keep it alive across subscribe/unsubscribe cycles. Must be paired with `UnlockSignature`. |
| `UnlockSignature(Tag)` | Removes the lock. If the container has no remaining subscribers after unlocking, it is removed from the active event map. |

#### Broadcasting and Removal

| Method | Description |
|--------|-------------|
| `Broadcast<TArgs...>(Tag, Args...)` | Broadcasts to all delegates registered under the given tag. For complex argument types (`const&`, pointers), template arguments may need to be specified explicitly to match the original signature. |
| `Remove(Tag, Handle)` | Removes a single delegate by handle. If this leaves the container empty and unlocked, the container is cleaned up. |
| `RemoveAll(Tag, Object)` | Removes all delegates bound to the given object. |

#### Query Methods

| Method | Description |
|--------|-------------|
| `IsBound(Tag)` | Returns `true` if at least one delegate is bound to the given tag. |
| `IsBoundToObject(Tag, Object)` | Returns `true` if at least one delegate bound to the tag is owned by the specified object. |
| `IsArgsType<TArgs...>(Tag)` | Returns `true` if the type signature currently associated with the tag matches the provided types. |

#### Type System Internals

The `TEventContainer` uses a compile-time FNV-1a hash (`consteval`) of the type names produced by `__PRETTY_FUNCTION__` / `__FUNCSIG__` as a unique type identifier. This gives O(1) type comparison at runtime while producing human-readable diagnostic messages on signature mismatches.

---

### Linq

> Header: `Linq.h`

A LINQ-style query library providing fluent, lazy-evaluated operations on Unreal Engine containers (`TArray`, `TSet`, `TMap`) and standard arrays.

#### Architecture

The system is built on a **CRTP iterator chain**. Intermediate operations construct new iterator layers without executing anything. Evaluation is deferred until a terminal operation consumes the chain.

| Layer | Description |
|-------|-------------|
| `ILinqIterator<Derived, T>` | Base CRTP interface defining `Next()`, `Reset()`, and `GetCurrent()`. |
| `TLValueSourceIterator` | Iterator for L-Value collections (borrows a pointer to the source). |
| `TRValueSourceIterator` | Iterator for R-Value collections (takes ownership via `MoveTemp`). |
| `TStorageTraits` | Helper to determine how to store intermediate values, handling the distinction between raw values and pointers to references. |

#### Intermediate Operations (Lazy)

| Operation | Description |
|-----------|-------------|
| `Where(Predicate)` | Filters elements based on a predicate. |
| `Select(Projection)` | Projects/transforms each element into a new form (Map operation). |
| `Apply(Modifier)` | Applies a modification function to each element and returns the modified element. Useful for side-effects that change object state. |
| `Execute(Function)` | Executes a function on each element without modifying the return flow. Pure side-effect iterator (like a `ForEach` that continues the chain). |
| `OrderBy()` | Sorts using the default `operator<`. |
| `OrderBy(Comparator)` | Sorts using a custom binary predicate. |
| `OrderBy(KeySelector)` | Sorts by a projected key. |
| `Cast<DerivedType>()` | Attempts to cast UObject pointers to a specific type. Only returns elements where the cast succeeds. |
| `IsValid()` | Filters UObject pointers, returning only those that satisfy `::IsValid()`. |
| `Skip(N)` | Bypasses the first N elements. |
| `Take(N)` | Returns only the first N elements. |

> **Note**: `OrderBy` variants buffer the entire collection before iterating, unlike filtering iterators which operate element-by-element.

#### Terminal Operations (Immediate)

| Operation | Description |
|-----------|-------------|
| `First(Predicate)` | Returns the first element satisfying the predicate, or `NullOpt`. |
| `FirstOrDefault(Predicate, Default)` | Returns the first match, or a default value. |
| `Last(Predicate)` | Returns the last element satisfying the predicate, or `NullOpt`. |
| `LastOrDefault(Predicate, Default)` | Returns the last match, or a default value. |
| `Single(Predicate)` | Returns the single matching element. Asserts if 0 or more than 1 found. |
| `Count(Predicate)` | Counts elements satisfying the predicate. |
| `Sum(Selector)` / `Sum()` | Sums projected values or arithmetic elements directly. |
| `Min(Comparator)` / `Min()` | Finds the minimum element. |
| `Max(Comparator)` / `Max()` | Finds the maximum element. |
| `Any(Predicate)` | Checks if any element satisfies the predicate. |
| `All(Predicate)` | Checks if all elements satisfy the predicate. |
| `Contains(Value, Comparator)` / `Contains(Value)` | Checks if the sequence contains a specific value. |
| `ToArray()` | Materializes the sequence into a `TArray`. |
| `ToSet()` | Materializes the sequence into a `TSet`. |
| `ToMap()` | Materializes into a `TMap` (from pair elements). |
| `ToMap(KeySelector)` | Materializes into a `TMap` using a key selector. |
| `ToMap(KeySelector, ValueSelector)` | Materializes into a `TMap` using both key and value selectors. |
| `Evaluate()` | Consumes the iterator to the end (useful for executing side-effects). |

#### Entry Point

```cpp
// Start a LINQ query from any container
auto Results = Linq::From(MyArray)
    .Where([](const FItem& Item) { return Item.bIsActive; })
    .Select([](const FItem& Item) { return Item.Name; })
    .OrderBy()
    .ToArray();
```

---

### TimerHolder

> Header: `TimerHolder.h`

A wrapper that contains a timer handle and uses the **RAII idiom** to automatically clear the scheduled timer upon destruction.

#### Configuration

```cpp
struct FTimerParameters
{
    bool bIsLooping { false };
    float Rate      { 0.f };
    float FirstDelay{ -1.f };
};
```

#### API

| Method | Description |
|--------|-------------|
| `Schedule(Callback, Params)` | Schedules a timer with a functor callback. Constrained by `Concept::Invocable`. |
| `Schedule(Object, Callback, Params)` | Schedules a timer with a UObject member function callback. Constrained by `Concept::DerivedFromObject` and `Concept::Invocable`. |
| `Pause()` | Pauses the running timer. |
| `Clear()` | Cancels and clears the timer. |
| `IsPaused()` | Returns `true` if the timer is currently paused. |
| `IsAlreadyRunning()` | Returns `true` if the timer is active. |
| `GetElapsedTime()` | Returns the elapsed time since the timer started. |
| `GetRate()` | Returns the configured rate. |
| `GetRemainingTime()` | Returns the remaining time until the next fire. |

The timer manager is resolved at runtime from the current game world or, when `WITH_EDITOR` is enabled, from the editor world context. If no valid timer manager is found, an `ensureMsgf` is triggered.

---

### Pool System

> Headers: `PoolSystem/PoolSubsystem.h`, `PoolSystem/PoolObject.h`, `PoolSystem/Poolable.h`, `PoolSystem/PoolSettings.h`

An actor object pooling system that pre-allocates, reuses, and dynamically manages pooled actors to avoid runtime spawn/destroy overhead.

#### Configuration: `FPoolSettings`

| Property | Type | Description |
|----------|------|-------------|
| `SpawnClass` | `TSubclassOf<AActor>` | The actor class to spawn. Must implement the `IPoolable` interface. |
| `WorldContext` | `UObject*` | Cached pointer to the world context used for spawning actors. Typically set at runtime during initialization. |
| `bAllowResize` | `bool` | If `true`, the pool automatically creates new chunks of objects when it runs out of free ones. If `false`, `SpawnFromPool` returns `nullptr` when the pool is empty. |
| `bAutoShrink` | `bool` | If `true`, the pool periodically checks for unused objects that exceed `MinPoolSize` and destroys them to free memory. |
| `MinPoolSize` | `int32` | The initial number of objects to spawn when the pool is initialized. The pool will never shrink below this count. |
| `AutoShrinkUpdateTime` | `float` | The interval (in seconds) at which the shrink routine checks for objects to remove. Only relevant if `bAutoShrink` is `true`. |
| `ShrinkObjectAfterReturnTime` | `float` | The duration (in seconds) an object must remain unused (in the "Free" state) before it is eligible for destruction during a shrink pass. |

A `UPoolSettingsDataAsset` wrapper is provided for creating and storing pool configurations as assets in the Content Browser.

#### IPoolable Interface

Any actor managed by the pool **must** implement `IPoolable`:

| Method | Description |
|--------|-------------|
| `OnSpawn()` | Called when the object is retrieved from the pool. Use this to reset state (health, ammo, position) instead of `BeginPlay`, which is only called once upon creation. |
| `OnReturn()` | Called when the object is returned to the pool. Use this to clean up active effects, timers, or references before the object is deactivated. |
| `Internal_GetIndex()` | Returns the internal pool index. Used by the pool manager for O(1) removal. |
| `Internal_SetIndex(NewIndex)` | Sets the internal pool index. Should only be called by the pool manager. |

#### UPoolSubsystem

The central manager for creating and accessing object pools within a world. As a `UWorldSubsystem`, its lifecycle is tied to the `UWorld`.

```cpp
// Get the subsystem and create a pool
auto* PoolSub = GetWorld()->GetSubsystem<UPoolSubsystem>();
UPoolObject* Pool = PoolSub->CreatePool(MyPoolSettings);

// Or create from a Data Asset
UPoolObject* Pool = PoolSub->CreatePoolFromDataAsset(MyPoolSettingsAsset);
```

#### UPoolObject

Manages a pool of specific actors. Handles spawning, retrieving, returning, and dynamically resizing.

| Method | Description |
|--------|-------------|
| `SetupPoolObject(Settings)` | Initializes the pool and pre-warms it by spawning the initial batch of actors. |
| `SpawnFromPool(Transform)` | Retrieves an actor from the pool and places it at the specified transform. If the pool is empty and expandable, new objects are created. Returns `nullptr` if the pool is full and fixed-size. |
| `SpawnFromPool<T>(Transform)` | Templated convenience wrapper that automatically casts the result to the specific actor type. |
| `CanSpawn()` | Returns `true` if there are free objects or if the pool is allowed to grow. |
| `ReturnToPool(Actor)` | Returns an active actor back to the pool. Deactivates the actor (hides, disables tick, disables collision) and makes it available for future spawns. |

<details>
<summary><strong>Internal Mechanics</strong></summary>

- **State Switching**: `SwitchActorState` toggles visibility, ticking, and collision between `Activate` and `Deactivate` states.
- **Index Queue**: A `TQueue` stores indices of returned objects, providing O(1) retrieval.
- **Chunk Allocation**: `AddNewChunk` instantiates batches of actors defined by `MinPoolSize`.
- **Shrink Routine**: A periodic timer checks for objects that have remained in the "Free" state longer than `ShrinkObjectAfterReturnTime` and destroys them, keeping the pool at or above `MinPoolSize`.

</details>

---

### Save System

> Headers: `SaveSystem/SaveSubsystem.h`, `SaveSystem/SaveComponent.h`, `SaveSystem/SaveData.h`, `SaveSystem/Saveable.h`

A full actor/component serialization and persistence system with support for version migration, dynamic and static actor handling, and binary property serialization.

#### Data Structures

##### `FPropertySaveData`

Serialized representation of a single `UPROPERTY` marked with the `SaveGame` flag.

| Field | Type | Description |
|-------|------|-------------|
| `PropertyName` | `FName` | Name of the property (must match the `UPROPERTY` `FName` on the owning class). |
| `PropertyType` | `FString` | C++ type string used to detect type mismatches when loading (e.g., `"int32"`, `"FVector"`). |
| `PropertyData` | `TArray<uint8>` | Raw binary payload produced by `FStructuredArchive` serialization. |

##### `FComponentSaveData`

Saved state of a single actor component that derives from `USaveableComponent`.

| Field | Type | Description |
|-------|------|-------------|
| `ComponentID` | `FString` | Unique identifier of the component within its owning actor. |
| `SaveVersion` | `FString` | Version string at the time of serialization, used to trigger migration logic on load. |
| `Properties` | `TArray<FPropertySaveData>` | Serialized properties for this component. |
| `PropertiesCount` | `int32` | Expected property count, used to detect possible data corruption. |

##### `FObjectSaveData`

Complete saved state of an actor that has a `USaveComponent` attached.

| Field | Type | Description |
|-------|------|-------------|
| `ObjectID` | `FString` | Unique identifier (`FGuid` string for dynamic actors, actor name for static ones). |
| `ObjectClass` | `UClass*` | Class used to spawn or locate the actor during deserialization. |
| `SaveVersion` | `FString` | Version string at the time of serialization. |
| `SpawnTransform` | `FTransform` | World transform to restore when re-spawning a dynamic actor. |
| `bIsDynamicSpawned` | `bool` | If `true`, the actor was runtime-spawned and will be re-created on load. Otherwise it is found in the level. |
| `Properties` | `TArray<FPropertySaveData>` | Serialized actor properties. |
| `PropertiesCount` | `int32` | Expected property count (corruption detection). |
| `Components` | `TArray<FComponentSaveData>` | Saved component data for all `USaveableComponent` sub-components. |
| `ComponentsCount` | `int32` | Expected component count (corruption detection). |

##### `ULCUSaveGame`

Root `USaveGame` object that holds the entire world save state, serialized via `UGameplayStatics`.

| Field | Type | Description |
|-------|------|-------------|
| `GlobalSaveVersion` | `FString` | Global version string (semantic versioning: `"Major.Minor.Patch"`). |
| `SaveTimeStamp` | `FDateTime` | Timestamp of when the save was created. |
| `SaveSlotName` | `FString` | Slot name this save was written to. |
| `ObjectsData` | `TArray<FObjectSaveData>` | All serialized actor data contained in this save. |

#### USaveComponent

Actor component that marks its owning actor as saveable. Attach this component to any actor to include it in the save system. The serializer saves the owning actor's `UPROPERTY(SaveGame)` properties, not the properties of this component itself.

All functions are `BlueprintNativeEvent` or `BlueprintCallable`, making this component fully usable from Blueprints.

| Method | Description |
|--------|-------------|
| `SetIsDynamicSpawned(SpawnClass, UID)` | Marks the owning actor as dynamically spawned so it will be re-created on load. A new `FGuid` is generated if no UID is provided. |
| `GetUniqueID()` | Returns the unique identifier (GUID for dynamic, actor name for static). |
| `GetDynamicSpawnClass()` | Returns the class used to re-spawn the owning actor if it is dynamic. |
| `GetIsDynamicSpawned()` | Returns `true` if the owning actor was spawned at runtime. |
| `GetSaveVersion()` | Returns the current save version string. Override in subclass to provide a version. Compared against the saved version on load to trigger migration. |
| `SetupSaveMigrateLogic()` | Entry point for registering version migration delegates. Called once on each class CDO during `USaveSubsystem::Initialize`. Override to register migration steps. |
| `AddMigrateDelegate(From, To, Delegate)` | Registers a Blueprint migration delegate for a specific version transition. |
| `AddMigrateDelegateLambda(From, To, Lambda)` | Registers a C++ lambda as a migration delegate. |
| `GetMigrateDelegateMap()` | Returns the migration delegate map for the given component's class. |
| `OnPreSave()` / `OnPostSave()` | Serialization lifecycle callbacks. |
| `OnPreLoad()` / `OnPostLoad()` | Deserialization lifecycle callbacks. |

#### USaveableComponent

Base class for actor components whose `UPROPERTY(SaveGame)` properties should be serialized alongside their owning actor. Provides independent versioning and migration support per component class. The owning actor must also have a `USaveComponent` attached for the save system to discover this component.

The API mirrors `USaveComponent` for versioning and migration (`GetSaveVersion`, `SetupSaveMigrateLogic`, `AddMigrateDelegate`, lifecycle callbacks).

#### FSaveSerializer

Static utility struct responsible for converting actors and their components to and from `FObjectSaveData`. Handles property iteration, binary serialization via `FStructuredArchive`, type checking, and version migration dispatch.

| Method | Description |
|--------|-------------|
| `SerializeActor(SaveComp)` | Serializes an actor and all its `USaveableComponents` into an `FObjectSaveData`. Iterates over all `UPROPERTY(SaveGame)` on the actor and its saveable components, captures version information, spawn data, and transform. |
| `DeserializeActor(WorldContext, Data, Version, StaticMap)` | Reconstructs an actor from saved data. For dynamic actors, spawns a new instance. For static actors, finds the existing actor by ID. Runs the migration delegate chain when the saved version differs from the current one, then restores all properties and component data. |

#### USaveSubsystem

`UGameInstanceSubsystem` serving as the main entry point for the save system.

| Method | Description |
|--------|-------------|
| `SaveWorld(SlotName, Version)` | Discovers all actors that have a `USaveComponent`, serializes them and their `USaveableComponents`, then writes the result to disk via `UGameplayStatics::SaveGameToSlot`. Version must follow the format `"Major.Minor.Patch"`. |
| `LoadWorld(SlotName, Version)` | Reads the save file, then for each saved actor entry calls `DeserializeActor` which handles spawning/finding actors, migration, and property restoration. |
| `ExtractVersion(Version)` | Parses a `"Major.Minor.Patch"` version string into a numeric tuple. Returns `NullOpt` if the format is invalid. |
| `BuildStaticActorSpawnedMap()` | Builds a map of `UniqueID -> AActor*` for all non-dynamic saveable actors in the world. |

On initialization, the subsystem iterates over all loaded classes deriving from `USaveComponent` or `USaveableComponent` and calls `SetupSaveMigrateLogic` on their CDOs to populate the per-class migration delegate maps.

<details>
<summary><strong>Version Migration</strong></summary>

Version migration uses a **per-class static delegate map**. Override `SetupSaveMigrateLogic` in a subclass (C++ or Blueprint) and call `AddMigrateDelegate` / `AddMigrateDelegateLambda` to register migration steps:

```cpp
void UMySaveComponent::SetupSaveMigrateLogic_Implementation()
{
    AddMigrateDelegateLambda(
        TEXT("1.0.0"), TEXT("2.0.0"),
        [](AActor* Actor, FString From, FString To, const TArray<FPropertySaveData>& OldProps)
        {
            // Remap or default values as needed
        }
    );
}
```

During deserialization, when the saved version does not match the current version, the registered delegate receives the actor being migrated, the source and target version strings, and the old property array so migration logic can remap or default values.

</details>

---

### Runtime Property Editor

> Headers: `RuntimePropertyEditor/RuntimePropertyEditor.h`, `RuntimePropertyEditor/RuntimePropertyEditorSubsystem.h`, `RuntimePropertyEditor/PropertyBuilder.h`, `RuntimePropertyEditor/RuntimeEditable.h`, `RuntimePropertyEditor/RuntimePropertyHelper.h`, `RuntimePropertyEditor/SlateFontHelper.h`, `RuntimePropertyEditor/RuntimePropertyEditorSettings.h`

A Slate-based property editor designed primarily for **packaged builds**. In a packaged build, Unreal Engine's editor tools (Details panel, property inspectors) are no longer available, making it impossible to inspect or tweak object properties on the fly. The Runtime Property Editor fills that gap by providing a dedicated OS-level window that lets you browse registered objects and modify their exposed properties in real time, directly inside the running game. Objects must be explicitly registered via `RegisterEditableObject` and implement the `IRuntimeEditable` interface to appear in the editor.

While the system technically works in the editor as well, it offers little practical value there since the built-in Details panel already provides full property editing capabilities. The real utility of this module emerges in packaged builds, where it becomes the only way to inspect and adjust the properties of actors and objects without restarting the application.

Typical use cases include runtime debugging of gameplay variables, live-tuning of parameters during QA sessions, and providing designers with a lightweight in-game inspection tool that does not require the full Unreal Editor.

#### Architecture

```
 URuntimePropertyEditorSubsystem (UWorldSubsystem)
   Owns the editor window, registered objects, and UI state

 SRuntimePropertyEditor (Slate Widget)
   Two-panel interface:
   +------------+---------------------+
   | ListView   | Properties Panel    |
   | (Objects)  | (Selected Object)   |
   |            |                     |
   | Object1    | +---------------+   |
   | Object2    | | Properties    |   |
   | Object3    | | ScrollBox     |   |
   |            | +---------------+   |
   +------------+---------------------+
   Panels separated by a resizable splitter.

 FRuntimePropertyBuilder (Fluent API Builder)
   Constructs property widgets without requiring Slate knowledge

 IRuntimeEditable (Interface)
   Implemented by objects that want to expose properties
```

#### IRuntimeEditable Interface

Objects implementing this interface can define which properties should be exposed and how they should be displayed. This interface must be implemented in C++ (Blueprint implementation is not supported).

```cpp
class AMyActor : public AActor, public IRuntimeEditable
{
    virtual void OnPropertiesDisplay(FRuntimePropertyBuilder& Builder) override
    {
        Builder
            .AddCategory("Movement")
            .AddNumeric<float>("Speed",
                [this]() { return Speed; },
                [this](float Val) { Speed = Val; })
            .AddButton("Reset", [this]() { ResetSpeed(); });
    }
};
```

#### FRuntimePropertyBuilder

Fluent API builder for constructing runtime property editor UIs. All `Add*` methods return a reference to `*this`, allowing method chaining.

| Method | Description |
|--------|-------------|
| `AddNumeric<T>(Name, Getter, Setter)` | Adds a numeric property editor (spin box) for `int32`, `float`, `double`, etc. |
| `AddNumericVector<T, N>(Name, Getter, Setter)` | Adds a vector property editor with X, Y, Z fields. Supports `FVector`, `FVector2D`. |
| `AddNumericRotator<T>(Name, Getter, Setter)` | Adds a rotator property editor with Pitch, Yaw, Roll fields. |
| `AddBool(Name, Getter, Setter)` | Adds a boolean property editor (checkbox). |
| `AddButton(Name, OnClicked)` | Adds an action button that executes a callback when clicked. |
| `AddCategory(Name)` | Adds a visual category header (centered, bold text) to organize properties. |
| `AddSeparator(Color, Thickness)` | Adds a horizontal separator line to visually divide sections. |
| `AddRowProperty(Widget)` | Adds a full-width custom Slate widget. |
| `AddRowProperty(Name, Widget)` | Adds a labeled property row (label on left, widget on right). |

#### URuntimePropertyEditorSubsystem

`UWorldSubsystem` managing the editor lifecycle and state. One instance per `UWorld`.

| Method | Description |
|--------|-------------|
| `Get(WorldContext)` | Returns the subsystem instance for the given world context, or `nullptr`. |
| `OpenWindow()` | Opens the editor as a native OS window (not an in-game widget). Does nothing if already open. |
| `CloseWindow()` | Closes the window and hides the selection box. Registered objects are **not** unregistered. |
| `RegisterEditableObject(Object)` | Registers an object implementing `IRuntimeEditable` to appear in the editor. |
| `UnRegisterEditableObject(Object)` | Unregisters an object and clears its cached property widgets. Clears the properties panel if the object was selected. |

When an actor is selected in the editor, a **selection box** (wireframe cube mesh) is spawned in the world around it to provide visual feedback. The box follows the actor's transform with a 0.1 scale unit padding.

#### URuntimePropertyHelper

Blueprint Function Library wrapping subsystem functionality in convenient static functions:

| Method | Description |
|--------|-------------|
| `OpenWindow(WorldContext)` | Opens the editor window using the Chain pattern for null safety. |
| `CloseWindow(WorldContext)` | Closes the editor window. |
| `RegisterEditableObject(WorldContext, Object)` | Registers an object. |
| `UnRegisterEditableObject(WorldContext, Object)` | Unregisters an object. |

#### URuntimePropertyEditorSettings

Project settings accessible via **Project Settings > Plugins > Runtime Property Editor**:

| Property | Default | Description |
|----------|---------|-------------|
| `SelectionMaterial` | `/LogicraftCoreUtils/Materials/RuntimePropertyEditor/Mat_Selection_Inst` | Material used for the selection box. Should be translucent or wireframe. |
| `SelectionMesh` | `/Engine/BasicShapes/Cube` | Static mesh used for the selection box. |

#### SlateFontStyleHelper

Utility class for loading fonts in packaged builds. `FAppStyle::GetFontStyle()` is editor-only and crashes in packaged builds; this helper provides safe alternatives.

| Method | Description |
|--------|-------------|
| `GetFontStyle(Path, Size)` | Loads a font from an absolute file path. **No validation** -- will crash if the path is invalid. |
| `GetFontStyleSafe(Path, Size)` | Loads a font with path validation. Falls back to Roboto-Regular if the file does not exist. |
| `GetSlateFontStyle(Name, Size)` | Loads a Roboto font from Unreal's `Engine/Content/Slate/Fonts` directory. **No validation**. |
| `GetSlateFontStyleSafe(Name, Size)` | **(Recommended)** Loads a Roboto font with existence validation. Falls back to Roboto-Regular on failure. |

Available engine fonts: `Roboto-Regular.ttf`, `Roboto-Bold.ttf`, `Roboto-Italic.ttf`, `Roboto-BoldItalic.ttf`, `Roboto-Light.ttf`, `Roboto-Medium.ttf`.

---

## Meta Utilities

> Headers: `Meta/LCUConcepts.h`, `Meta/LCUTypeTraits.h`

### Concepts (`Concept` namespace)

| Concept | Description |
|---------|-------------|
| `IsDelegate<T>` | Checks if the type is an Unreal `TDelegate`. |
| `DerivedFromObject<T>` | Checks if the type inherits from `UObject` (standard for UE garbage collection). Uses both `std::derived_from` and `std::is_convertible_v`. |
| `DerivedFromActor<T>` | Checks if the type inherits from `AActor` (can be placed in the world). |
| `Invocable<TCallable, TArgs...>` | Checks if `TCallable` can be invoked with `TArgs...`. Direct wrapper around C++20 `std::invocable`. |

### Type Traits (`TypeTrait` namespace)

| Trait | Description |
|-------|-------------|
| `TArgsTrait<TArgs...>` | Helper for variadic template arguments. Stores types in a `TTuple` and provides indexed access via `TArgType<Index>`. |
| `TFunctionTraits<T>` | Analyzes function signatures. Deduces return type, owner class, and argument types. Specializations cover member functions (const and non-const), static/global functions, and callable objects (functors/lambdas). |
| `TIsDelegate<T>` | Trait to detect if a type matches the `TDelegate<Signature>` pattern. Inherits from `std::true_type` or `std::false_type`. |
| `bIsDelegate_V<T>` | Helper variable template for `TIsDelegate`. |

---

## Log Categories

The plugin defines three log categories, all with `Warning` as the default verbosity level:

| Category | Usage |
|----------|-------|
| `LogLCU` | General plugin logging. |
| `LogEventBus` | EventBus-specific logging. |
| `LogSaveSystem` | SaveSystem-specific logging. |

---

## License

Copyright (c) Logicraft Interactive. All Rights Reserved.
