// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#pragma once

#include <string_view>
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Meta/LCUConcepts.h"
#include "Meta/LCUTypeTraits.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EventBus.generated.h"

/**
 * Define this to make the delegates themselves thread-safe at the cost of performance.
 *
 * When enabled, each TEventContainer will use FDefaultTSDelegateUserPolicy, which
 * protects the delegate's internal invocation list with a FTransactionallySafeCriticalSection
 * (a true system mutex, not a spinlock). Note that UE's implementation uses the same mutex
 * for both reads and writes, meaning concurrent reads are also mutually exclusive.
 *
 * WARNING: Because reads and writes share the same mutex, high-frequency broadcasts
 * or heavy callbacks will serialize all concurrent accesses and may hurt performance.
 * Only enable this if you truly need concurrent Add/Remove and Broadcast on the
 * same tag from different threads.
 *
 * By default this is disabled: the EventBus RWLock already protects the active
 * event map, which is sufficient for the typical GameThread usage pattern.
 */
#define EVENTBUS_THREAD_SAFE_DELEGATES 0

// ---------------------------------------------------------------------------
// GetTypeNameAsString
// ---------------------------------------------------------------------------

/**
 * Extracts the type name of T as a compile-time string view.
 *
 * Uses compiler-specific macros (__PRETTY_FUNCTION__, __FUNCSIG__) to parse
 * the type name at compile time. This is used to produce human-readable
 * type mismatch diagnostics in the EventBus ensure messages.
 */
template <typename T>
consteval std::basic_string_view<TCHAR> GetTypeNameAsString()
{
	using FString_View_TChar = std::basic_string_view<TCHAR>;
	
#if defined(__clang__)
	constexpr FString_View_TChar Function = TEXT(__PRETTY_FUNCTION__);
	constexpr auto Prefix = TEXT("std::basic_string_view<TCHAR> GetTypeNameAsString() [T = ");
	constexpr auto Suffix = TEXT("]");
#elif defined(__GNUC__)
	constexpr FString_View_TChar Function = std::basic_string_view<TCHAR>(TEXT(__PRETTY_FUNCTION__));
	constexpr auto Prefix = TEXT("[with T = ");
	constexpr auto Suffix = TEXT(";");
#elif defined(_MSC_VER)
	constexpr FString_View_TChar Function = TEXT(__FUNCSIG__);
	constexpr auto Prefix = TEXT("GetTypeNameAsString<");
	constexpr auto Suffix = TEXT(">(void)");
#else
	return TEXT("Unknown compiler");
#endif
	
	constexpr auto StartLoc = Function.find(Prefix);
	if constexpr (StartLoc == FString_View_TChar::npos)
	{
		return TEXT("ErrorParsing");
	}

	constexpr auto Start = StartLoc + std::char_traits<TCHAR>::length(Prefix);
	constexpr auto End = Function.find(Suffix, Start);
    
	if constexpr (End == FString_View_TChar::npos)
	{
		return Function.substr(Start);
	}

	return Function.substr(Start, End - Start);
}

// ---------------------------------------------------------------------------
// IEventContainerBase
// ---------------------------------------------------------------------------

/**
 * Type-erased interface for an event container.
 *
 * Allows the EventBus to store and manipulate containers of any delegate
 * signature through a common pointer, without knowing the concrete TArgs.
 *
 * The TypeID mechanism (StaticGetTypeID / GetTypeID) provides a lightweight
 * runtime type check to catch signature mismatches.
 */
struct IEventContainerBase
{
	IEventContainerBase() = default;
	virtual ~IEventContainerBase() = default;

	/** Returns a unique identifier for the concrete container type. Used for runtime type checking. */
	virtual const uint64 GetTypeID() = 0;
	
	/** Removes a single delegate by handle. Returns true if found and removed. */
	virtual bool Remove(FDelegateHandle DelegateHandle) = 0;

	/** Removes all delegates bound to the given object. Returns the number of delegates removed. */
	virtual int32 RemoveAll(const void* UserObject) = 0;

	/** Returns true if at least one delegate is bound. */
	virtual bool IsBound() const = 0;

	/** Returns true if at least one delegate is bound to the given object. */
	virtual bool IsBoundToObject(const void* UserObject) const = 0;

	/**
	 * Increments the subscriber count.
	 * The subscriber count tracks how many delegates are registered, independently
	 * of the delegate's own internal state, to allow cleanup decisions.
	 */
	virtual int32 AddSubscriber() = 0;

	/** Decrements the subscriber count. */
	virtual int32 RemoveSubscriber() = 0;

	/** Returns the current number of registered subscribers. */
	virtual int32 GetSubscriberCount() const = 0;

	/**
	 * Locks or unlocks the type signature for this container.
	 *
	 * When locked, the container persists in the ActiveEvents map even with zero
	 * subscribers, preserving the type association for the gameplay tag.
	 */
	virtual void SetLockedSignature(bool InLockedSignature) = 0;

	/** Returns true if the signature is currently locked. */
	virtual bool GetLockedSignature() const = 0;

	/** Returns a human-readable string of the template argument types. Used in diagnostics. */
	virtual FString GetTypesName() const = 0;
};

// ---------------------------------------------------------------------------
// TEventContainer
// ---------------------------------------------------------------------------

/**
 * Concrete event container for a specific delegate signature <TArgs...>.
 *
 * Holds the actual TMulticastDelegate and the subscriber count.
 * The TypeID is a per-instantiation static address, giving O(1) type comparison.
 *
 * Thread-safety of the delegate itself is controlled by EVENTBUS_THREAD_SAFE_DELEGATES:
 * - 0 (default): standard TMulticastDelegate, no internal locking.
 * - 1: FDefaultTSDelegateUserPolicy is applied, protecting the delegate's invocation
 * list with a FTransactionallySafeCriticalSection. Both reads and writes share
 * the same mutex, so concurrent broadcasts are also serialized.
 */
template<typename ...TArgs>
struct TEventContainer final : IEventContainerBase
{
	/** Number of delegates currently registered through the EventBus. */
	int32 SubscriberCount{ 0 };

#if EVENTBUS_THREAD_SAFE_DELEGATES
	/** Thread-safe multicast delegate. */
	TMulticastDelegate<void(TArgs...), FDefaultTSDelegateUserPolicy> MulticastDelegate;
#else
	/** Standard multicast delegate (no internal locking). */
	TMulticastDelegate<void(TArgs...)> MulticastDelegate;
#endif
	
	/** Whether the type signature is locked, preventing cleanup on zero subscribers. */
	bool bLockedSignature{ false };
	
	/**
	 * Computes a 64-bit FNV-1a hash of a string view entirely at compile time.
	 * Uses a standard for-loop which is fully supported in consteval and 
	 * avoids the massive compile-time overhead of template recursion.
	 */
	static consteval uint64 CompileTimeStringHash(std::basic_string_view<TCHAR> StringView)
	{
		constexpr uint64 FNV_Offset_Basis{ 0xcbf29ce484222325ull };
		constexpr uint64 FNV_Prime{ 0x100000001b3ull };

		uint64 Hash{ FNV_Offset_Basis };

		for (size_t Index = 0; Index < StringView.size(); ++Index)
		{
			Hash ^= static_cast<uint64>(StringView[Index]);
			Hash *= FNV_Prime;
		}

		return Hash;
	}
	
	/**
	 * Returns a unique type identifier for this TEventContainer instantiation.
	 */
	static consteval uint64 StaticGetTypeID()
	{
		uint64 CombinedHash{ 0 };

		([&]
		{
			constexpr uint64 TypeHash{ CompileTimeStringHash(GetTypeNameAsString<TArgs>()) };
			
			CombinedHash ^= TypeHash + 0x9e3779b97f4a7c15ull + (CombinedHash << 6) + (CombinedHash >> 2);
		}(), ...);

		return CombinedHash;
	}
	
	virtual const uint64 GetTypeID() override { return StaticGetTypeID(); }
	
	virtual bool Remove(FDelegateHandle DelegateHandle) override
	{
		if (MulticastDelegate.Remove(DelegateHandle))
		{
			RemoveSubscriber();
			return true;
		}

		return false;
	}
	
	virtual int32 RemoveAll(const void* UserObject) override
	{
		const int32 RemovedSubscriber{ MulticastDelegate.RemoveAll(UserObject) };
		SubscriberCount -= RemovedSubscriber;
		return RemovedSubscriber;
	}	
	
	virtual bool IsBound() const override { return MulticastDelegate.IsBound(); }
	virtual bool IsBoundToObject(const void* UserObject) const override { return MulticastDelegate.IsBoundToObject(UserObject); }
	
	virtual int32 AddSubscriber() override { return ++SubscriberCount; }
	virtual int32 RemoveSubscriber() override { return --SubscriberCount; }
	virtual int32 GetSubscriberCount() const override { return SubscriberCount; }

	virtual void SetLockedSignature(bool InLockedSignature) override { bLockedSignature = InLockedSignature; }
	virtual bool GetLockedSignature() const override { return bLockedSignature; }

	/** Builds a comma-separated string of all TArgs type names. Used in diagnostics. */
	static FString StaticGetTypesName()
	{
		TArray<FString> TypeNames;
		
		([&]
		{
			constexpr auto View = GetTypeNameAsString<TArgs>();
			TypeNames.Emplace(FString(View.length(), View.data()));
		}(), ...);
		
		return FString::Join(TypeNames, TEXT(", "));
	}
	
	virtual FString GetTypesName() const override
	{
		return StaticGetTypesName();
	}
};

// ---------------------------------------------------------------------------
// EventBus namespace — TypeTraits & Concepts
// ---------------------------------------------------------------------------

namespace EventBus
{
	namespace TypeTraits
	{
		/**
		 * Primary template: not a functor by default.
		 * Specialized below for member function pointers matched against a TTuple of argument types.
		 */
		template<typename, typename...>
		struct TIsFunctor
		{
			constexpr static bool bValue = false;
		};

		/** Specialization for non-const member functions whose arguments match TArgs. */
		template<typename TReturn, typename TClass, typename ...TArgs>
		struct TIsFunctor<TReturn (TClass::*)(TArgs...), TTuple<TArgs...>>
		{
			constexpr static bool bValue = std::is_invocable_v<TReturn (TClass::*)(TArgs...), TClass, TArgs...>;
		};

		/** Specialization for const member functions whose arguments match TArgs. */
		template<typename TReturn, typename TClass, typename ...TArgs>
		struct TIsFunctor<TReturn (TClass::*)(TArgs...) const, TTuple<TArgs...>>
		{
			constexpr static bool bValue = std::is_invocable_v<TReturn (TClass::*)(TArgs...) const, TClass, TArgs...>;
		};

		/** Helper variable template for TIsFunctor. */
		template<typename TMemberFunction, typename ...TArgs>
		constexpr static bool TIsFunctor_V = TIsFunctor<TMemberFunction, TArgs...>::bValue;

		/**
		 * Maps a list of types to the corresponding TEventContainer instantiation.
		 * Three specializations handle: raw TArgs..., TTuple<TArgs...>, and TDelegate<void(TArgs...)>.
		 */
		template<typename ...TArgs>
		struct FContainerTypeFor
		{
			using FType = TEventContainer<TArgs...>;
		};
	
		template<typename ...TArgs>
		struct FContainerTypeFor<TTuple<TArgs...>>
		{
			using FType = TEventContainer<TArgs...>;
		};

		template<typename ...TArgs>
		struct FContainerTypeFor<TDelegate<void(TArgs...)>>
		{
			using FType = TEventContainer<TArgs...>;
		};

		/** Alias to directly obtain the container type from a type list. */
		template<typename ...TArgs>
		using TContainerTypeFor = FContainerTypeFor<TArgs...>::FType;

	} // namespace TypeTraits
	
	namespace Concepts
	{
		/**
		 * Satisfied when TFunctor is a callable object (e.g. lambda) whose operator()
		 * signature matches its deduced argument types.
		 */
		template<typename TFunctor>
		concept IsFunctor =
			TypeTraits::TIsFunctor_V<
				decltype(&TFunctor::operator()), typename TypeTrait::TFunctionTraits<TFunctor>::FArgsType>;

	} // namespace Concepts

} // namespace EventBus

// ---------------------------------------------------------------------------
// UEventBus
// ---------------------------------------------------------------------------

/**
 * A type-safe, thread-safe Event Bus that maps Gameplay Tags to multicast delegates.
 *
 * This subsystem facilitates decoupling by allowing disparate systems to communicate
 * via Tags without direct references.
 *
 * --- Runtime Strict Typing ---
 * The first delegate bound to a Gameplay Tag determines the expected signature
 * (argument types) for that Tag. All subsequent bindings or broadcasts must match
 * this signature. The signature is released once all subscribers are removed
 * (unless explicitly locked via LockSignature).
 *
 * --- Thread-Safety ---
 * The internal ActiveEvents map is protected by an FRWLock:
 * - All write operations (Add, Remove, LockSignature, UnlockSignature) acquire
 * a write lock for the duration of the map access.
 * - Read operations (IsBound, IsBoundToObject, IsArgsType) acquire a read lock.
 * - Broadcast acquires a short read lock only to extract the TSharedPtr to the
 * container, then releases the lock before invoking callbacks. This prevents
 * deadlocks when a callback re-enters the EventBus (e.g. calls Add or Remove).
 * The TSharedPtr keeps the container alive even if another thread removes it
 * from the map between the unlock and the broadcast.
 *
 * The delegate invocation lists themselves are NOT protected by default.
 * Concurrent Add/Remove and Broadcast on the same tag from different threads
 * is undefined behavior unless EVENTBUS_THREAD_SAFE_DELEGATES is enabled.
 * In practice, all operations are expected to occur on the GameThread.
 */
UCLASS()
class LOGICRAFTCOREUTILS_API UEventBus : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
	/** Maps each Gameplay Tag to its type-erased event container. */
	TMap<FGameplayTag, TSharedRef<IEventContainerBase>> ActiveEvents;
	
	/**
	 * Protects ActiveEvents against concurrent access.
	 * Non-recursive: never acquire this lock from a function that already holds it.
	 */
	mutable FRWLock RWLock;

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	/** Returns the EventBus subsystem for the world associated with WorldContext, or nullptr. */
	static ThisClass* Get(const UObject* WorldContext);

	// -----------------------------------------------------------------------
	// Add* — Register delegates
	// -----------------------------------------------------------------------

	/**
	 * Adds a UObject-based member function delegate.
	 * Stores a weak reference to UserObject; the delegate is automatically
	 * skipped during broadcast if the object has been garbage collected.
	 *
	 * @param GameplayTag   Tag identifying the event to subscribe to.
	 * @param UserObject    The UObject instance to bind to.
	 * @param Function      Member function pointer on UserObject.
	 * @return              A handle that can be passed to Remove().
	 */
	template<typename TUserObject, typename TMemberFunction>
		requires std::is_base_of_v<UObject, TUserObject> &&
			std::is_member_function_pointer_v<TMemberFunction>
	FDelegateHandle AddUObject(const FGameplayTag& GameplayTag, TUserObject* UserObject, TMemberFunction Function)
	{
		return Internal_AddCallback<typename TypeTrait::TFunctionTraits<TMemberFunction>::FArgsType>(GameplayTag,
		[UserObject, Function](auto& EventContainer)
		{
			return EventContainer.MulticastDelegate.AddUObject(UserObject, Function);
		});
	}

	/**
	 * TObjectPtr overload of AddUObject.
	 * Unwraps the TObjectPtr and forwards to the raw pointer overload.
	 *
	 * @param GameplayTag   Tag identifying the event to subscribe to.
	 * @param UserObject    The TObjectPtr to the UObject instance.
	 * @param Function      Member function pointer on UserObject.
	 * @return              A handle that can be passed to Remove().
	 */
	template<typename TUserObject, typename TMemberFunction>
		requires std::is_base_of_v<UObject, TUserObject> &&
			std::is_member_function_pointer_v<TMemberFunction>
	FDelegateHandle AddUObject(const FGameplayTag& GameplayTag, TObjectPtr<TUserObject> UserObject, TMemberFunction Function)
	{
		return AddUObject(GameplayTag, UserObject.Get(), Function);
	}

	/**
	 * Adds a UFunction-based delegate (reflected function, callable from Blueprint).
	 * Stores a weak reference to UserObject.
	 *
	 * @param GameplayTag    Tag identifying the event to subscribe to.
	 * @param UserObject     The UObject instance that owns the UFunction.
	 * @param FunctionName   Name of the UFunction to bind.
	 * @tparam TFunctionArgs Explicit argument types of the UFunction signature.
	 * @return               A handle that can be passed to Remove().
	 */
	template<typename TUserObject, typename ...TFunctionArgs>
		requires std::is_base_of_v<UObject, TUserObject>
	FDelegateHandle AddUFunction(const FGameplayTag& GameplayTag, TUserObject* UserObject, const FName& FunctionName)
	{
		return Internal_AddCallback<TFunctionArgs...>(GameplayTag,
		[UserObject, &FunctionName](auto& EventContainer)
		{
			return EventContainer.MulticastDelegate.AddUFunction(UserObject, FunctionName);
		});
	}

	/**
	 * TObjectPtr overload of AddUFunction.
	 * Unwraps the TObjectPtr and forwards to the raw pointer overload.
	 *
	 * @param GameplayTag    Tag identifying the event to subscribe to.
	 * @param UserObject     The TObjectPtr to the UObject instance.
	 * @param FunctionName   Name of the UFunction to bind.
	 * @tparam TFunctionArgs Explicit argument types of the UFunction signature.
	 * @return               A handle that can be passed to Remove().
	 */
	template<typename TUserObject, typename ...TFunctionArgs>
		requires std::is_base_of_v<UObject, TUserObject>
	FDelegateHandle AddUFunction(const FGameplayTag& GameplayTag, TObjectPtr<TUserObject> UserObject, const FName& FunctionName)
	{
		return AddUFunction<TUserObject, TFunctionArgs...>(GameplayTag, UserObject.Get(), FunctionName);
	}
	
	/**
	 * Adds a pre-built TDelegate instance to the invocation list.
	 * The delegate type determines the expected signature for the tag.
	 *
	 * @param GameplayTag   Tag identifying the event to subscribe to.
	 * @param Delegate      The delegate instance to add (moved into the list).
	 * @return              A handle that can be passed to Remove().
	 */
	template<Concept::IsDelegate TDelegate>
	FDelegateHandle Add(const FGameplayTag& GameplayTag, TDelegate&& Delegate)
	{
		return Internal_AddCallback<typename TRemoveReference<TDelegate>::Type>(GameplayTag,
		[]<typename TCallbackDelegate>(auto& EventContainer, TCallbackDelegate&& CallbackDelegate)
		{
			return EventContainer.MulticastDelegate.Add(Forward<TCallbackDelegate>(CallbackDelegate));
		},
		Forward<TDelegate>(Delegate));
	}

	/**
	 * Adds a weak lambda delegate bound to a UObject.
	 * The lambda will not be invoked if UserObject has been garbage collected.
	 *
	 * @param GameplayTag   Tag identifying the event to subscribe to.
	 * @param UserObject    The UObject whose lifetime gates the lambda invocation.
	 * @param Functor       Lambda or functor to bind.
	 * @return              A handle that can be passed to Remove().
	 */
	template<typename TUserObject, EventBus::Concepts::IsFunctor TFunctor>
		requires std::is_base_of_v<UObject, TUserObject>
	FDelegateHandle AddWeakLambda(const FGameplayTag& GameplayTag, TUserObject* UserObject, TFunctor&& Functor)
	{
		return Internal_AddCallback<typename TypeTrait::TFunctionTraits<TFunctor>::FArgsType>(GameplayTag,
		[UserObject]<typename TCallbackFunctor>(auto& EventContainer, TCallbackFunctor&& CallbackFunctor)
		{
			return EventContainer.MulticastDelegate.AddWeakLambda(UserObject, Forward<TCallbackFunctor>(CallbackFunctor));
		},
		Forward<TFunctor>(Functor));
	}

	/**
	 * Adds a raw lambda delegate with no lifetime management.
	 * The lambda is always invoked as long as it is registered.
	 * Caller is responsible for removing it before it becomes invalid.
	 *
	 * @param GameplayTag   Tag identifying the event to subscribe to.
	 * @param Functor       Lambda or functor to bind.
	 * @return              A handle that can be passed to Remove().
	 */
	template<EventBus::Concepts::IsFunctor TFunctor>
	FDelegateHandle AddLambda(const FGameplayTag& GameplayTag, TFunctor&& Functor)
	{
		return Internal_AddCallback<typename TypeTrait::TFunctionTraits<TFunctor>::FArgsType>(GameplayTag,
		[]<typename TCallbackFunctor>(auto& EventContainer, TCallbackFunctor&& CallbackFunctor)
		{
			return EventContainer.MulticastDelegate.AddLambda(Forward<TCallbackFunctor>(CallbackFunctor));
		},
		Forward<TFunctor>(Functor));	
	}

	/**
	 * Adds a raw C++ pointer member function delegate.
	 * No lifetime management: if the object is destroyed before the delegate
	 * is removed, invoking it is undefined behavior.
	 *
	 * @param GameplayTag   Tag identifying the event to subscribe to.
	 * @param UserObject    Raw pointer to the object instance (must not be a UObject).
	 * @param Function      Member function pointer on UserObject.
	 * @return              A handle that can be passed to Remove().
	 */
	template<typename TUserClass, typename TMemberFunction>
		requires !std::is_base_of_v<UObject, TUserClass> && std::is_member_function_pointer_v<TMemberFunction>
	FDelegateHandle AddRaw(const FGameplayTag& GameplayTag, TUserClass* UserObject, TMemberFunction Function)
	{
		return Internal_AddCallback<typename TypeTrait::TFunctionTraits<TMemberFunction>::FArgsType>(GameplayTag,
		[UserObject, Function](auto& EventContainer)
		{
			return EventContainer.MulticastDelegate.AddRaw(UserObject, Function);
		});	
	}

	/**
	 * Adds a shared pointer-based member function delegate (TSharedRef overload).
	 * Stores a weak reference; the delegate is skipped if the shared object expires.
	 * Automatically selects AddThreadSafeSP when Mode == ESPMode::ThreadSafe.
	 *
	 * @param GameplayTag    Tag identifying the event to subscribe to.
	 * @param UserObjectRef  Shared reference to the object instance.
	 * @param Function       Member function pointer on the shared object.
	 * @return               A handle that can be passed to Remove().
	 */
	template<typename TSharedRefType, ESPMode Mode, typename TMemberFunction>
		requires std::is_member_function_pointer_v<TMemberFunction>
	FDelegateHandle AddSP(const FGameplayTag& GameplayTag, const TSharedRef<TSharedRefType, Mode>& UserObjectRef, TMemberFunction Function)
	{
		return Internal_AddCallback<typename TypeTrait::TFunctionTraits<TMemberFunction>::FArgsType>(GameplayTag,
		[&UserObjectRef, Function](auto& EventContainer)
		{
			if constexpr (Mode == ESPMode::ThreadSafe)
			{
				return EventContainer.MulticastDelegate.AddThreadSafeSP(UserObjectRef, Function);
			}
			else
			{
				return EventContainer.MulticastDelegate.AddSP(UserObjectRef, Function);
			}
		});
	}

	/**
	 * Adds a shared pointer-based member function delegate (raw pointer overload).
	 * UserObject must inherit from TSharedFromThis<TSharedType, Mode>.
	 * Internally calls AsShared() to obtain the shared reference.
	 *
	 * @param GameplayTag   Tag identifying the event to subscribe to.
	 * @param UserObject    Raw pointer to a TSharedFromThis-derived instance.
	 * @param Function      Member function pointer on UserObject.
	 * @return              A handle that can be passed to Remove().
	 */
	template<typename TSharedType, ESPMode Mode, typename TMemberFunction>
		requires std::is_base_of_v<TSharedFromThis<TSharedType, Mode>, TSharedType> &&
			std::is_member_function_pointer_v<TMemberFunction>
	FDelegateHandle AddSP(const FGameplayTag& GameplayTag, TSharedType* UserObject, TMemberFunction Function)
	{
		return AddSP(GameplayTag, StaticCastSharedRef<TSharedType, Mode>(UserObject->AsShared()), Function);
	}
	
	/**
	 * Adds a weak shared pointer lambda delegate.
	 * The lambda is skipped during broadcast if the shared object has expired.
	 * UserObject must inherit from TSharedFromThis<TSharedType, Mode>.
	 *
	 * @param GameplayTag    Tag identifying the event to subscribe to.
	 * @param UserObjectRef  Raw pointer to a TSharedFromThis-derived instance.
	 * @param Functor        Lambda or functor to bind.
	 * @return               A handle that can be passed to Remove().
	 */
	template<typename TSharedType, ESPMode Mode, EventBus::Concepts::IsFunctor TFunctor>
		requires std::is_base_of_v<TSharedFromThis<TSharedType, Mode>, TSharedType>
	FDelegateHandle AddSPLambda(const FGameplayTag& GameplayTag, const TSharedType* UserObjectRef, TFunctor&& Functor)
	{
		return Internal_AddCallback<typename TypeTrait::TFunctionTraits<TFunctor>::FArgsType>(GameplayTag,
		[&UserObjectRef]<typename TCallbackFunctor>(auto& EventContainer, TCallbackFunctor&& CallbackFunctor)
		{
			return EventContainer.MulticastDelegate.AddSPLambda(UserObjectRef, Forward<TCallbackFunctor>(CallbackFunctor));
		},
		Forward<TFunctor>(Functor));
	}

	/**
	 * Adds a static (free) function delegate.
	 * No object binding; the function is always invoked unconditionally.
	 *
	 * @param GameplayTag   Tag identifying the event to subscribe to.
	 * @param Function      Static or free function pointer.
	 * @return              A handle that can be passed to Remove().
	 */
	template<typename TFunction>
		requires !TypeTrait::TFunctionTraits<TFunction>::bIsClass
	FDelegateHandle AddStatic(const FGameplayTag& GameplayTag, TFunction Function)
	{
		return Internal_AddCallback<typename TypeTrait::TFunctionTraits<TFunction>::FArgsType>(GameplayTag,
		[Function](auto& EventContainer)
		{
			return EventContainer.MulticastDelegate.AddStatic(Function);
		});	
	}

	// -----------------------------------------------------------------------
	// Signature locking
	// -----------------------------------------------------------------------

	/**
	 * Locks the type signature for the given Gameplay Tag.
	 *
	 * Pre-associates a specific delegate signature <TArgs...> with the tag and
	 * prevents the container from being removed when its subscriber count reaches zero.
	 * This is useful to reserve a tag's signature before any subscriber registers,
	 * or to keep it alive across subscribe/unsubscribe cycles.
	 *
	 * If a container already exists for the tag, its type is verified against TArgs.
	 * If no container exists, one is created and immediately locked.
	 *
	 * Must be paired with a call to UnlockSignature when the lock is no longer needed.
	 *
	 * @tparam TArgs       Expected argument types for this tag's delegate signature.
	 
	 * @param GameplayTag  Tag whose signature to lock.
	 */
	template<typename ...TArgs>
	void LockSignature(const FGameplayTag& GameplayTag)
	{
		using FEventContainerType = EventBus::TypeTraits::TContainerTypeFor<TArgs...>;

		FWriteScopeLock WriteScopeLock{ RWLock };
		if (const auto BaseEventContainer{ Internal_Find(GameplayTag) })
		{
			if (Internal_CheckTypesID<FEventContainerType>(BaseEventContainer, TEXT("The gameplay tag already has an assigned signature.")))
			{
				BaseEventContainer->SetLockedSignature(true);
			}

			return;
		}

		FEventContainerType& EventContainer{ Internal_AddActiveEvent<FEventContainerType>(GameplayTag) };
		EventContainer.SetLockedSignature(true);
	}

	/**
	 * Unlocks the type signature for the given Gameplay Tag.
	 *
	 * Removes the lock set by LockSignature. If the container has no remaining
	 * subscribers after unlocking, it is removed from the active event map.
	 *
	 
	 * @param GameplayTag  Tag whose signature to unlock.
	 */
	void UnlockSignature(const FGameplayTag& GameplayTag);

	// -----------------------------------------------------------------------
	// Broadcast
	// -----------------------------------------------------------------------

	/**
	 * Broadcasts to all delegates registered under the given Gameplay Tag.
	 *
	 * Thread-safety strategy:
	 * 1. A short read lock is acquired to retrieve a TSharedPtr to the container.
	 * 2. The lock is released immediately after.
	 * 3. The broadcast happens outside the lock, preventing deadlocks when a
	 * callback re-enters the EventBus (e.g. calls Add or Remove).
	 * 4. The TSharedPtr keeps the container alive even if another thread removes
	 * it from the map between the unlock and the actual broadcast.
	 *
	 * Internally, UE's TMulticastDelegate::Broadcast locks the invocation list via
	 * an internal counter (LockInvocationList/UnlockInvocationList) rather than copying it.
	 * Re-entrant modifications (Add/Remove during broadcast) are deferred: the list is
	 * iterated on the original reference, and compaction only happens after UnlockInvocationList.
	 *
	 
	 * @param GameplayTag   Tag identifying the event to broadcast.
	 * @param Args          Arguments forwarded to all bound delegates.
	 *
	 * @note For complex argument types (const refs, pointers), TArgs may be deduced
	 * differently from the signature used at bind time, causing a type mismatch ensure.
	 * In that case, specify the template arguments explicitly to match the original signature:
	 * EventBus->Broadcast<const FMyStruct&>(Tag, MyStruct);
	 * The explicit types must exactly match those used in the Add* call.
	 */
	template<typename ...TArgs>
	void Broadcast(const FGameplayTag& GameplayTag, TArgs... Args)
	{
		using FEventContainerType = EventBus::TypeTraits::TContainerTypeFor<TArgs...>;
		
		TSharedPtr<IEventContainerBase> BaseEventContainer;
		{
			FReadScopeLock ReadScopeLock{ RWLock };
			BaseEventContainer = Internal_Find(GameplayTag);
		}
		
		if (BaseEventContainer)
		{
			if (Internal_CheckTypesID<FEventContainerType>(BaseEventContainer, TEXT("Unable to broadcast a callback with those types of arguments.")))
			{
				auto& EventContainer = static_cast<FEventContainerType&>(*BaseEventContainer);
				EventContainer.MulticastDelegate.Broadcast(Args...);
			}
		}
	}

	// -----------------------------------------------------------------------
	// Remove
	// -----------------------------------------------------------------------

	/**
	 * Removes a single delegate from the invocation list by handle (O(N)).
	 * The order of remaining delegates may not be preserved.
	 * If removing this delegate leaves the container empty and unlocked,
	 * the container is removed from the active event map.
	 *
	 
	 * @param GameplayTag   Tag identifying the event to unsubscribe from.
	 * @param Handle        Handle returned by a previous Add* call.
	 * @return              True if the delegate was found and removed.
	 */
	bool Remove(const FGameplayTag& GameplayTag, FDelegateHandle Handle);

	/**
	 * Removes all delegates bound to the given object from the invocation list.
	 * The order of remaining delegates may not be preserved.
	 * If removing leaves the container empty and unlocked, it is cleaned up.
	 *
	 
	 * @param GameplayTag   Tag identifying the event to unsubscribe from.
	 * @param UserObject    The object whose delegates should be removed.
	 * @return              The number of delegates removed.
	 */
	int32 RemoveAll(const FGameplayTag& GameplayTag, const void* UserObject);

	// -----------------------------------------------------------------------
	// Query
	// -----------------------------------------------------------------------

	/**
	 * Returns true if at least one delegate is currently bound to the given tag.
	 *
	 
	 * @param GameplayTag   Tag to check.
	 */
	bool IsBound(const FGameplayTag& GameplayTag) const;

	/**
	 * Returns true if at least one delegate bound to the given tag is owned by UserObject.
	 *
	 
	 * @param GameplayTag   Tag to check.
	 * @param UserObject    The object to check binding for.
	 */
	bool IsBoundToObject(const FGameplayTag& GameplayTag, const void* UserObject) const;

	/**
	 * Returns true if the type signature currently associated with the given tag
	 * matches the provided TArgs types.
	 *
	 * Useful to verify compatibility before broadcasting or binding without
	 * triggering an ensure on mismatch.
	 *
	 * @tparam TArgs        Argument types to check against the tag's current signature.
	 
	 * @param GameplayTag   Tag to check.
	 */
	template<typename ...TArgs>
	bool IsArgsType(const FGameplayTag& GameplayTag) const
	{
		using FEventContainerType = EventBus::TypeTraits::TContainerTypeFor<TArgs...>;
		
		FReadScopeLock ReadScopeLock{ RWLock };
		if (const auto BaseEventContainer{ Internal_Find(GameplayTag) })
		{
			return BaseEventContainer->GetTypeID() == FEventContainerType::StaticGetTypeID();
		}

		return false;
	}
	
private:

	// -----------------------------------------------------------------------
	// Internal helpers
	// -----------------------------------------------------------------------

	/**
	 * Core implementation shared by all Add* variants.
	 *
	 * Acquires a write lock, then either:
	 * - Finds the existing container for GameplayTag, verifies its type matches
	 * FEventContainerType, invokes Callable to bind the delegate, and increments
	 * the subscriber count.
	 * - Creates a new container if none exists, then does the same.
	 *
	 * Callable receives a typed TEventContainer<TContainerArgs> reference and any
	 * additional Args, and must return an FDelegateHandle.
	 *
	 * NOTE: Internal_Find and Internal_AddActiveEvent must never acquire RWLock
	 * themselves, as this function already holds a write lock (FRWLock is non-recursive).
	 */
	template<typename TContainerArgs, typename TCallable, typename ...TArgs>
		requires std::invocable<TCallable, EventBus::TypeTraits::TContainerTypeFor<TContainerArgs>&, TArgs...>
	FDelegateHandle Internal_AddCallback(const FGameplayTag& GameplayTag, TCallable&& Callable, TArgs&&... Args)
	{
		using FEventContainerType = EventBus::TypeTraits::TContainerTypeFor<TContainerArgs>;

		FWriteScopeLock WriteScopeLock{ RWLock };
		if (const auto BaseEventContainer{ Internal_Find(GameplayTag) })
		{
			if (Internal_CheckTypesID<FEventContainerType>(BaseEventContainer, TEXT("Unable to add a callback of a different type.")))
			{
				auto& EventContainer = static_cast<FEventContainerType&>(*BaseEventContainer);
				const FDelegateHandle DelegateHandle{ Callable(EventContainer, Forward<TArgs>(Args)...) };

				if (DelegateHandle.IsValid())
				{
					EventContainer.AddSubscriber();
					return DelegateHandle;
				}
			}
			
			return {};
		}

		FEventContainerType& EventContainer{ Internal_AddActiveEvent<FEventContainerType>(GameplayTag) };
		const FDelegateHandle DelegateHandle{ Callable(EventContainer, Forward<TArgs>(Args)...) };
		if (!DelegateHandle.IsValid())
		{
			return {};
		}
		
		EventContainer.AddSubscriber();
		return DelegateHandle;
	}

	/**
	 * Inserts a new TEventContainerType into ActiveEvents for the given tag and returns a reference to it.
	 * Must be called while holding a write lock on RWLock.
	 */
	template<typename TEventContainerType>
	TEventContainerType& Internal_AddActiveEvent(const FGameplayTag& GameplayTag)
	{
		return static_cast<TEventContainerType&>(ActiveEvents.Add(GameplayTag, MakeShared<TEventContainerType>()).Get());
	}

	/**
	 * Returns a TSharedPtr to the container associated with the given tag, or nullptr.
	 * Must be called while holding at least a read lock on RWLock.
	 */
	TSharedPtr<IEventContainerBase> Internal_Find(const FGameplayTag& GameplayTag) const;

	/**
	 * Removes the container associated with the given tag from ActiveEvents.
	 * Returns true if an entry was found and removed.
	 * Must be called while holding a write lock on RWLock.
	 */
	bool Internal_Remove(const FGameplayTag& GameplayTag);

	/**
	 * Verifies that ActualType's runtime TypeID matches TExpectedType's static TypeID.
	 * Triggers an ensure with a diagnostic message on mismatch, listing both
	 * the expected and actual argument type names.
	 *
	 * Returns true if types match, false otherwise.
	 */
	template<typename TExpectedType>
	static bool Internal_CheckTypesID(
		const TSharedPtr<IEventContainerBase>& ActualType,
		const FStringView EnsureMessage)
	{
		const uint64 ExpectedTypeId = TExpectedType::StaticGetTypeID();
		const uint64 ActualTypeId   = ActualType->GetTypeID();
		
		return ensureMsgf(
			ExpectedTypeId == ActualTypeId,								  					    
				TEXT("%s\n"
			 		"[Given Types: %s]\n"  										  					  		
			 		"[Wanted Types: %s]"),
			 		EnsureMessage.GetData(),			 											  					  	
			 		*ActualType->GetTypesName(),					  					  		
			 		*TExpectedType::StaticGetTypesName());
	}
};