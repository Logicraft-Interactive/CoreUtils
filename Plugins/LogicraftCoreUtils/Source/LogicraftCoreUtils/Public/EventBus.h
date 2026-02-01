// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "EventBus.generated.h"

namespace TypeTraits
{
	template<typename ...TArgs>
	struct TArgsTrait
	{
		using FArgs = TTuple<TArgs...>;

		template<size_t Index>
		using TArg = TTupleElement<Index, FArgs>;
	};

	template<typename Type>
	struct TFunctionTraits : TFunctionTraits<decltype(&Type::operator())>
	{};

	template<typename TReturn, typename TClass, typename ...TArgs>
	struct TFunctionTraits<TReturn (TClass::*)(TArgs...)>
	{
		static constexpr bool bIsClass{ true };
		static constexpr bool bIsConst{ false };

		using FReturnType = TReturn;
		using FClassType = TClass;
		using FArgsType = TArgsTrait<TArgs...>::FArgs;

		template<size_t Index>
		using TArgType = TArgsTrait<TArgs...>::template TArg<Index>::Type;

		using FFunctionType = TReturn (TClass::*)(TArgs...);
	};

	template<typename TReturn, typename TClass, typename ...TArgs>
	struct TFunctionTraits<TReturn (TClass::*)(TArgs...) const>
	{
		static constexpr bool bIsClass{ true };
		static constexpr bool bIsConst{ true };

		using FReturnType = TReturn;
		using FClassType = TClass;
		using FArgsType = TArgsTrait<TArgs...>::FArgs;

		template<size_t Index>
		using TArgType = TArgsTrait<TArgs...>::template TArg<Index>::Type;

		using FFunctionType = TReturn (TClass::*)(TArgs...) const;
	};

	template<typename TReturn, typename ...TArgs>
	struct TFunctionTraits<TReturn (*)(TArgs...)>
	{
		static constexpr bool bIsClass{ false };
		static constexpr bool bIsConst{ false };

		using FReturnType = TReturn;
		using FArgsType = TArgsTrait<TArgs...>::FArgs;

		template<size_t Index>
		using TArgType = TArgsTrait<TArgs...>::template TArg<Index>::Type;

		using FFunctionType = TReturn (*)(TArgs...);
	};
} // TypeTraits

struct IEventContainerBase
{
	IEventContainerBase() = default;
	virtual ~IEventContainerBase() = default;

	virtual const void* GetTypeID() = 0;
	
	virtual bool Remove(FDelegateHandle DelegateHandle) = 0;
	virtual int32 RemoveAll(const void* UserObject) = 0;
	virtual bool IsBound() const = 0;
	virtual bool IsBoundToObject(const void* UserObject) const = 0;

	virtual int32 AddSubscriber() = 0;
	virtual int32 RemoveSubscriber() = 0;
	virtual int32 GetSubscriberCount() const = 0;

	virtual void SetLockedSignature(bool LockedSignature) = 0;
	virtual bool GetLockedSignature() const = 0;
};

template<typename ...Args>
struct TEventContainer final : IEventContainerBase
{
	int32 SubscriberCount{ 0 };
	TMulticastDelegate<void(Args...)> MulticastDelegate;
	bool bLockedSignature{ false };

	static const void* StaticGetTypeID()
	{
		static char TypeId;
		return &TypeId;
	}
	
	virtual const void* GetTypeID() override { return StaticGetTypeID(); }
	
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
};

namespace EventBus
{
	namespace TypeTraits
	{
		template<typename, typename...>
		struct TIsFunctor
		{
			constexpr static bool bValue = false;
		};

		template<typename TReturn, typename TClass, typename ...TArgs>
		struct TIsFunctor<TReturn (TClass::*)(TArgs...), TTuple<TArgs...>>
		{
			constexpr static bool bValue = std::is_invocable_v<TReturn (TClass::*)(TArgs...), TClass, TArgs...>;
		};

		template<typename TReturn, typename TClass, typename ...TArgs>
		struct TIsFunctor<TReturn (TClass::*)(TArgs...) const, TTuple<TArgs...>>
		{
			constexpr static bool bValue = std::is_invocable_v<TReturn (TClass::*)(TArgs...) const, TClass, TArgs...>;
		};

		template<typename TMemberFunction, typename ...TArgs>
		constexpr static bool TIsFunctor_V = TIsFunctor<TMemberFunction, TArgs...>::bValue;

		template<typename>
		struct TIsDelegate : std::false_type {};

		template<typename TReturn, typename ...TArgs>
		struct TIsDelegate<TDelegate<TReturn(TArgs...)>> : std::true_type {};

		template<typename Type>
		constexpr static bool bIsDelegate_V = TIsDelegate<Type>::value;

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

		template<typename ...TArgs>
		using TContainerTypeFor = FContainerTypeFor<TArgs...>::FType;
	} // TypeTraits
	
	namespace Concepts
	{
		template<typename Type>
		concept IsDelegate = TypeTraits::bIsDelegate_V<typename TRemoveReference<Type>::Type>;

		template<typename TFunctor>
		concept IsFunctor =
			TypeTraits::TIsFunctor_V<
				decltype(&TFunctor::operator()), typename ::TypeTraits::TFunctionTraits<TFunctor>::FArgsType>;
	} // Concepts
} // EventBus

/**
 * A type-safe Event Bus that maps Gameplay Tags to multicast delegates.
 *
 * This subsystem facilitates decoupling by allowing disparate systems to communicate 
 * via Tags without direct references.
 *
 * @note Runtime Strict Typing: The first delegate bound to a Gameplay Tag determines the 
 * expected signature (arguments) for that Tag. All subsequent bindings or broadcasts 
 * must match this signature until all subscribers are removed. After all subscribers
 * are removed a new signature can be attributed.
 */
UCLASS()
class LOGICRAFTCOREUTILS_API UEventBus : public UWorldSubsystem
{
	GENERATED_BODY()

	TMap<FGameplayTag, TSharedRef<IEventContainerBase>> ActiveEvents;

public:
	static ThisClass* Get(const UObject* WorldContext);

	/**
	 * Adds a UObject-based member function delegate.
	 *
	 * UObject delegates keep a weak reference to your object.
	 *
	 * @param   WorldContext Context to get the world.
	 * @param   GameplayTag  Associated gameplay tag.
	 * @param   UserObject   User object to bind to
	 * @param   Function     Class method function address
	 */
	template<typename TUserObject, typename TMemberFunction>
		requires std::is_base_of_v<UObject, TUserObject> &&
			std::is_member_function_pointer_v<TMemberFunction>
	static FDelegateHandle AddUObject(const UObject* WorldContext, const FGameplayTag& GameplayTag, TUserObject* UserObject, TMemberFunction Function)
	{
		return Internal_ExecuteOnValidContext(WorldContext,
		[&GameplayTag, UserObject, Function](ThisClass* EventBus)
		{
			return EventBus->Internal_AddCallback<typename TypeTraits::TFunctionTraits<TMemberFunction>::FArgsType>(GameplayTag,
			[UserObject, Function](auto& EventContainer)
			{
				return EventContainer.MulticastDelegate.AddUObject(UserObject, Function);
			});
		});
	}

	/**
	 * Adds a UObject-based member function delegate.
	 *
	 * UObject delegates keep a weak reference to your object.
	 * 
	 * @param   WorldContext Context to get the world.
	 * @param	GameplayTag  Associated gameplay tag.
	 * @param	UserObject	 User object to bind to
	 * @param	Function	 Class method function address
	 */
	template<typename TUserObject, typename TMemberFunction>
		requires std::is_base_of_v<UObject, TUserObject> &&
			std::is_member_function_pointer_v<TMemberFunction>
	static FDelegateHandle AddUObject(const UObject* WorldContext, const FGameplayTag& GameplayTag, TObjectPtr<TUserObject> UserObject, TMemberFunction Function)
	{
		return AddUObject(WorldContext, GameplayTag, UserObject.Get(), Function);
	}

	/**
	 * Adds a UFunction-based member function delegate.
	 *
	 * UFunction delegates keep a weak reference to your object.
	 *
	 * @param   WorldContext Context to get the world.
	 * @param	GameplayTag  Associated gameplay tag.
	 * @param	UserObject	 User object to bind to
	 * @param	FunctionName Class method function address
	 */
	template<typename TUserObject, typename ...TFunctionArgs>
		requires std::is_base_of_v<UObject, TUserObject>
	static FDelegateHandle AddUFunction(const UObject* WorldContext, const FGameplayTag& GameplayTag, TUserObject* UserObject, const FName& FunctionName)
	{
		return Internal_ExecuteOnValidContext(WorldContext,
		[&GameplayTag, UserObject, &FunctionName](ThisClass* EventBus)
		{
			return EventBus->Internal_AddCallback<TFunctionArgs...>(GameplayTag,
			[UserObject, &FunctionName](auto& EventContainer)
			{
				return EventContainer.MulticastDelegate.AddUFunction(UserObject, FunctionName);
			});
		});
	}

	/**
	 * Adds a UFunction-based member function delegate.
	 *
	 * UFunction delegates keep a weak reference to your object.
	 *
	 * @param   WorldContext Context to get the world.
	 * @param	GameplayTag  Associated gameplay tag.
	 * @param	UserObject	 User object to bind to
	 * @param	FunctionName Class method function address
	 */
	template<typename TUserObject, typename ...TFunctionArgs>
		requires std::is_base_of_v<UObject, TUserObject>
	static FDelegateHandle AddUFunction(const UObject* WorldContext, const FGameplayTag& GameplayTag, TObjectPtr<TUserObject> UserObject, const FName& FunctionName)
	{
		return AddUFunction<TUserObject, TFunctionArgs...>(WorldContext, GameplayTag, UserObject.Get(), FunctionName);
	}
	
	/**
	 * Adds a delegate instance to this multicast delegate's invocation list.
	 *
	 * @param   WorldContext Context to get the world.
	 * @param   GameplayTag  Associated gameplay tag.
	 * @param   Delegate	 The delegate to add.
	 */
	template<EventBus::Concepts::IsDelegate TDelegate>
	static FDelegateHandle Add(const UObject* WorldContext, const FGameplayTag& GameplayTag, TDelegate&& Delegate)
	{
		return Internal_ExecuteOnValidContext(WorldContext,
		[&GameplayTag]<typename TInDelegate>(ThisClass* EventBus, TInDelegate&& InDelegate)
		{
			return EventBus->Internal_AddCallback<TRemoveReference<TDelegate>::Type>(GameplayTag,
			[]<typename TCallbackDelegate>(auto& EventContainer, TCallbackDelegate&& CallbackDelegate)
			{
				return EventContainer.MulticastDelegate.Add(Forward<TCallbackDelegate>(CallbackDelegate));
			},
			Forward<TInDelegate>(InDelegate));
		},
		Forward<TDelegate>(Delegate));
	}

	/**
	 * Adds a weak object C++ lambda delegate
	 * technically this works for any functor types, but lambdas are the primary use case
	 *
	 * @param   WorldContext Context to get the world.
	 * @param   GameplayTag  Associated gameplay tag.
	 * @param   UserObject   User object to bind to
	 * @param   Functor	     Functor (e.g. Lambda)
	 */
	template<typename TUserObject, EventBus::Concepts::IsFunctor TFunctor>
		requires std::is_base_of_v<UObject, TUserObject>
	static FDelegateHandle AddWeakLambda(const UObject* WorldContext, const FGameplayTag& GameplayTag, TUserObject* UserObject, TFunctor&& Functor)
	{
		return Internal_ExecuteOnValidContext(WorldContext,
		[&GameplayTag, UserObject]<typename TInFunctor>(ThisClass* EventBus, TInFunctor&& InFunctor)
		{
			return EventBus->Internal_AddCallback<typename TypeTraits::TFunctionTraits<TFunctor>::FArgsType>(GameplayTag,
			[UserObject]<typename TCallbackFunctor>(auto& EventContainer, TCallbackFunctor&& CallbackFunctor)
			{
				return EventContainer.MulticastDelegate.AddWeakLambda(UserObject, Forward<TCallbackFunctor>(CallbackFunctor));
			},
			Forward<TInFunctor>(InFunctor));
		},
		Forward<TFunctor>(Functor));
	}

	/**
	 * Adds a C++ lambda delegate
	 * technically this works for any functor types, but lambdas are the primary use case
	 *
	 * @param   WorldContext Context to get the world.
	 * @param   GameplayTag Associated gameplay tag.
	 * @param   Functor	  Functor (e.g. Lambda)
	 */
	template<EventBus::Concepts::IsFunctor TFunctor>
	static FDelegateHandle AddLambda(const UObject* WorldContext, const FGameplayTag& GameplayTag, TFunctor&& Functor)
	{
		return Internal_ExecuteOnValidContext(WorldContext,
		[&GameplayTag]<typename TInFunctor>(ThisClass* EventBus, TInFunctor&& InFunctor)
		{
			return EventBus->Internal_AddCallback<typename TypeTraits::TFunctionTraits<TFunctor>::FArgsType>(GameplayTag,
			[]<typename TCallbackFunctor>(auto& EventContainer, TCallbackFunctor&& CallbackFunctor)
			{
				return EventContainer.MulticastDelegate.AddLambda(Forward<TCallbackFunctor>(CallbackFunctor));
			},
			Forward<TInFunctor>(InFunctor));	
		},
		Forward<TFunctor>(Functor));
	}

	/**
	 * Adds a raw C++ pointer delegate.
	 *
	 * Raw pointer doesn't use any sort of reference, so may be unsafe to call if the object was
	 * deleted out from underneath your delegate. Be careful when calling Execute()!
	 *
	 * @param   WorldContext Context to get the world.
	 * @param   GameplayTag  Associated gameplay tag.
	 * @param   UserObject   User object to bind to
	 * @param   Function	 Class method function address
	 */
	template<typename TUserClass, typename TMemberFunction>
		requires !std::is_base_of_v<UObject, TUserClass> && std::is_member_function_pointer_v<TMemberFunction>
	static FDelegateHandle AddRaw(const UObject* WorldContext, const FGameplayTag& GameplayTag, TUserClass* UserObject, TMemberFunction Function)
	{
		return Internal_ExecuteOnValidContext(WorldContext,
		[&GameplayTag, UserObject, Function](ThisClass* EventBus)
		{
			return EventBus->Internal_AddCallback<typename TypeTraits::TFunctionTraits<TMemberFunction>::FArgsType>(GameplayTag,
			[UserObject, Function](auto& EventContainer)
			{
				return EventContainer.MulticastDelegate.AddRaw(UserObject, Function);
			});	
		});
	}

	/**
	 * Adds a shared pointer-based member function delegate.
	 *
	 * Shared pointer delegates keep a weak reference to your object.
	 *
	 * @param   WorldContext  Context to get the world.
	 * @param   GameplayTag   Associated gameplay tag.
	 * @param   UserObjectRef User object to bind to
	 * @param   Function	  Class method function address
	 */
	template<typename TSharedRefType, ESPMode Mode, typename TMemberFunction>
		requires std::is_member_function_pointer_v<TMemberFunction>
	static FDelegateHandle AddSP(const UObject* WorldContext, const FGameplayTag& GameplayTag, const TSharedRef<TSharedRefType, Mode>& UserObjectRef, TMemberFunction Function)
	{
		return Internal_ExecuteOnValidContext(WorldContext,
		[&GameplayTag, &UserObjectRef, Function](ThisClass* EventBus)
		{
			return EventBus->Internal_AddCallback<typename TypeTraits::TFunctionTraits<TMemberFunction>::FArgsType>(GameplayTag,
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
		});
	}

	/**
	 * Adds a shared pointer-based member function delegate.
	 *
	 * Shared pointer delegates keep a weak reference to your object.
	 *
	 * @param   WorldContext Context to get the world.
	 * @param   GameplayTag  Associated gameplay tag.
	 * @param   UserObject   User object to bind to
	 * @param   Function     Class method function address
	 */
	template<typename TSharedType, ESPMode Mode, typename TMemberFunction>
		requires std::is_base_of_v<TSharedFromThis<TSharedType, Mode>, TSharedType> &&
			std::is_member_function_pointer_v<TMemberFunction>
	static FDelegateHandle AddSP(const UObject* WorldContext, const FGameplayTag& GameplayTag, TSharedType* UserObject, TMemberFunction Function)
	{
		return AddSP(WorldContext, GameplayTag, StaticCastSharedRef<TSharedType, Mode>(UserObject->AsShared()), Function);
	}
	
	/**
	 * Adds a weak shared pointer C++ lambda delegate
	 * technically this works for any functor types, but lambdas are the primary use case
	 *
	 * @param   WorldContext  Context to get the world.
	 * @param   GameplayTag   Associated gameplay tag.
	 * @param   UserObjectRef User object to bind to
	 * @param   Functor		  Functor (e.g. Lambda)
	 */
	template<typename TSharedType, ESPMode Mode, EventBus::Concepts::IsFunctor TFunctor>
		requires std::is_base_of_v<TSharedFromThis<TSharedType, Mode>, TSharedType>
	static FDelegateHandle AddSPLambda(const UObject* WorldContext, const FGameplayTag& GameplayTag, const TSharedType* UserObjectRef, TFunctor&& Functor)
	{
		return Internal_ExecuteOnValidContext(WorldContext,
		[&GameplayTag, UserObjectRef]<typename TInFunctor>(ThisClass* EventBus, TInFunctor&& InFunctor)
		{
			return EventBus->Internal_AddCallback<typename TypeTraits::TFunctionTraits<TFunctor>::FArgsType>(GameplayTag,
			[&UserObjectRef]<typename TCallbackFunctor>(auto& EventContainer, TCallbackFunctor&& CallbackFunctor)
			{
				return EventContainer.MulticastDelegate.AddSPLambda(UserObjectRef, Forward<TCallbackFunctor>(CallbackFunctor));
			},
			Forward<TInFunctor>(InFunctor));
		},
		Forward<TFunctor>(Functor));
	}

	/**
	 * Adds a raw C++ pointer global function delegate
	 *
	 * @param   WorldContext Context to get the world.
	 * @param   GameplayTag  Associated gameplay tag.
	 * @param   Function	 Function pointer
	 */
	template<typename TFunction>
		requires !TypeTraits::TFunctionTraits<TFunction>::bIsClass
	static FDelegateHandle AddStatic(const UObject* WorldContext, const FGameplayTag& GameplayTag, TFunction Function)
	{
		return Internal_ExecuteOnValidContext(WorldContext,
		[&GameplayTag, Function](ThisClass* EventBus)
		{
			return EventBus->Internal_AddCallback<typename TypeTraits::TFunctionTraits<TFunction>::FArgsType>(GameplayTag,
			[Function](auto& EventContainer)
            {
            	return EventContainer.MulticastDelegate.AddStatic(Function);
            });	
		});
	}

	/**
	 * Lock a specific type signature.
	 * This pre-associates types with gameplay tags, preventing any "add event" function from setting the signature.
	 *
	 * @tparam TArgs       Types of the signature.
	 * @param WorldContext Context to get the world.
	 * @param GameplayTag  Associated gameplay tag.
	 */
	template<typename ...TArgs>
	static void LockSignature(const UObject* WorldContext, const FGameplayTag& GameplayTag)
	{
		Internal_ExecuteOnValidContext(WorldContext, [&GameplayTag](ThisClass* EventBus)
		{
			using FEventContainerType = EventBus::TypeTraits::TContainerTypeFor<TArgs...>;

			if (const auto BaseEventContainer{ EventBus->Internal_Find(GameplayTag) })
			{
				const auto ExpectedTypeId = FEventContainerType::StaticGetTypeID();
				const auto ActualTypeId = BaseEventContainer->GetTypeID();
				if (ensureMsgf(ActualTypeId == ExpectedTypeId, TEXT("The gameplay tag has already an assigned signature.")))
				{
					BaseEventContainer->SetLockedSignature(true);
				}

				return;
			}

			FEventContainerType& EventContainer{ EventBus->Internal_AddActiveEvent<FEventContainerType>(GameplayTag) };
			EventContainer.bLockedSignature = true;
		});
	}

	/**
	 * Unlock a specific type signature.
	 * 
	 * @param WorldContext Context to get the world
	 * @param GameplayTag  Associated gameplay tag.
	 */
	static void UnLockSignature(const UObject* WorldContext, const FGameplayTag& GameplayTag);

	/**
	 * Broadcasts this delegate to all bound objects, except to those that may have expired.
	 *
	 * The constness of this method allows for broadcasting from const functions.
	 *
	 * @param   WorldContext Context to get the world.
	 * @param   GameplayTag  Associated gameplay tag.
	 * @param   Args		 Callback arguments.
	 */
	template<typename ...TArgs>
	static void Broadcast(const UObject* WorldContext, const FGameplayTag& GameplayTag, TArgs&&... Args)
	{
		using FEventContainerType = EventBus::TypeTraits::TContainerTypeFor<TArgs...>;
		
		Internal_ExecuteOnValidContext(WorldContext,
		[&GameplayTag]<typename ...TInArgs>(const ThisClass* EventBus, TInArgs&&... InArgs)
		{
			if (const auto BaseEventContainer{ EventBus->Internal_Find(GameplayTag) })
			{
				const auto ExpectedTypeId = FEventContainerType::StaticGetTypeID();
				const auto ActualTypeId = BaseEventContainer->GetTypeID();
				if (ensureMsgf(ActualTypeId == ExpectedTypeId, TEXT("Unable to broadcast a callback with those types of arguments.")))
				{
					auto& EventContainer = static_cast<FEventContainerType&>(*BaseEventContainer);
					EventContainer.MulticastDelegate.Broadcast(Forward<TInArgs>(InArgs)...);
				}
			}
		}, Forward<TArgs>(Args)...);
	}

	/**
	 * Removes a delegate instance from this multi-cast delegate's invocation list (performance is O(N)).
	 *
	 * Note that the order of the delegate instances may not be preserved!
	 *
	 * @param   WorldContext Context to get the world.
	 * @param   GameplayTag  Associated gameplay tag.
	 * @param   Handle       The handle of the delegate instance to remove.
	 * @return  True if the delegate was successfully removed.
	 */
	static bool Remove(const UObject* WorldContext, const FGameplayTag& GameplayTag, FDelegateHandle Handle);

	/**
	 * Removes all functions from this multi-cast delegate's invocation list that are bound to the specified UserObject.
	 * Note that the order of the delegates may not be preserved!
	 *
	 * @param   WorldContext Context to get the world.
	 * @param   GameplayTag  Associated gameplay tag.
	 * @param   UserObject   The object to remove all delegates for.
	 * @return  The number of delegates successfully removed.
	 */
	static int32 RemoveAll(const UObject* WorldContext, const FGameplayTag& GameplayTag, const void* UserObject);

	/**
	 * Checks to see if any functions are bound to this multi-cast delegate.
	 *
	 * @param   WorldContext Context to get the world.
	 * @param   GameplayTag  Associated gameplay tag.
	 * @return True if any functions are bound, false otherwise.
	 */
	static bool IsBound(const UObject* WorldContext, const FGameplayTag& GameplayTag);

	/** 
	 * Checks to see if any functions are bound to the given user object.
	 *
	 * @param   WorldContext Context to get the world.
	 * @param   GameplayTag  Associated gameplay tag.
	 * @return True if any functions are bound to InUserObject, false otherwise.
	 */
	static bool IsBoundToObject(const UObject* WorldContext, const FGameplayTag& GameplayTag, const void* UserObject);

	/**
	 * Checks to see if the given types correspond to the current associated signature with the gameplay tag. 
	 *
	 * @param   WorldContext Context to get the world.
	 * @param   GameplayTag  Associated gameplay tag.
	 * @return  True if the types match.
	 */
	template<typename ...TArgs>
	static bool IsArgsType(const UObject* WorldContext, const FGameplayTag& GameplayTag)
	{
		using FEventContainerType = EventBus::TypeTraits::TContainerTypeFor<TArgs...>;
		
		return Internal_ExecuteOnValidContext(WorldContext, [&GameplayTag](const ThisClass* EventBus)
		{
			if (const auto BaseEventContainer{ EventBus->Internal_Find(GameplayTag) })
			{
				return BaseEventContainer->GetTypeID() == FEventContainerType::StaticGetTypeID();
			}

			return false;
		});
	}
	
private:
	/**
	 * This function exist to avoid doing an "if" everytime a context need to be checked.
	 */
	template<typename TCallable, typename ...TArgs>
		requires std::invocable<TCallable, ThisClass*, TArgs...>
	static auto Internal_ExecuteOnValidContext(const UObject* WorldContext, TCallable&& Callable, TArgs&&... Args)
	{
		using ReturnType = std::invoke_result_t<TCallable, ThisClass*, TArgs...>;
		
		if (ThisClass* EventBus{ Get(WorldContext) })
		{
			return Callable(EventBus, Forward<TArgs>(Args)...);
		}
		
		if constexpr (!std::is_void_v<ReturnType>)
		{
			return ReturnType{};
		}
	}

	/**
	 * This function exist to avoid doing the same thing over and over again since only two lines change between call.
	 * To handle this two lines of code that changed I use a functor.
	 */
	template<typename TContainerArgs, typename TCallable, typename ...TArgs>
		requires std::invocable<TCallable, EventBus::TypeTraits::TContainerTypeFor<TContainerArgs>&, TArgs...>
	FDelegateHandle Internal_AddCallback(const FGameplayTag& GameplayTag, TCallable&& Callable, TArgs&&... Args)
	{
		using FEventContainerType = EventBus::TypeTraits::TContainerTypeFor<TContainerArgs>;
		
		if (const auto BaseEventContainer{ Internal_Find(GameplayTag) })
		{
			const auto ExpectedTypeId = FEventContainerType::StaticGetTypeID();
			const auto ActualTypeId = BaseEventContainer->GetTypeID();
			if (ensureMsgf(ActualTypeId == ExpectedTypeId, TEXT("Unable to add a callback of a different type.")))
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

	template<typename TEventContainerType>
	TEventContainerType& Internal_AddActiveEvent(const FGameplayTag& GameplayTag)
	{
		return static_cast<TEventContainerType&>(ActiveEvents.Add(GameplayTag, MakeShared<TEventContainerType>()).Get());
	}

	TSharedPtr<IEventContainerBase> Internal_Find(const FGameplayTag& GameplayTag) const;

	bool Internal_Remove(const FGameplayTag& GameplayTag);
};
