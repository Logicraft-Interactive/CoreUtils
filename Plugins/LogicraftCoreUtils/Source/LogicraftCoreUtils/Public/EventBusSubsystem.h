// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "EventBusSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE(TestDelegate)

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
		using TArgType = TArgsTrait<TArgs...>::template TArg<Index>;

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
		using TArgType = TArgsTrait<TArgs...>::template TArg<Index>;

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
		using TArgType = TArgsTrait<TArgs...>::template TArg<Index>;

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

	virtual uint32 AddSubscriber() = 0;
	virtual uint32 RemoveSubscriber() = 0;
	virtual void SetSubscriberCount(const uint32 NewSubscriberCount) = 0;
	virtual uint32 GetSubscriberCount() const = 0;
};

template<typename ...Args>
struct TEventContainer final : IEventContainerBase
{
	uint32 SubscriberCount{ 0 };
	TMulticastDelegate<void(Args...)> MulticastDelegate;

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
	
	virtual uint32 AddSubscriber() override { return ++SubscriberCount; }
	virtual uint32 RemoveSubscriber() override { return --SubscriberCount; }
	virtual uint32 GetSubscriberCount() const override { return SubscriberCount; }
	virtual void SetSubscriberCount(const uint32 NewSubscriberCount) override { SubscriberCount = NewSubscriberCount; }
};

namespace EventBus
{
	namespace TypeTraits
	{
		namespace Private
		{
			template<typename, typename, typename...>
			struct TIsFunctor
			{
				using FFunctionType = void;
				constexpr static bool bValue = false;
			};

			template<typename TReturn, typename TClass, typename ...TArgs>
			struct TIsFunctor<TReturn (TClass::*)(TArgs...), TClass, TTuple<TArgs...>>
			{
				using FFunctionType = TReturn (TClass::*)(TArgs...);
				constexpr static bool bValue = std::is_invocable_v<FFunctionType, TClass, TArgs...>;
			};

			template<typename TReturn, typename TClass, typename ...TArgs>
			struct TIsFunctor<TReturn (TClass::*)(TArgs...) const, TClass, TTuple<TArgs...>>
			{
				using FFunctionType = TReturn (TClass::*)(TArgs...) const;
				constexpr static bool bValue = std::is_invocable_v<FFunctionType, TClass, TArgs...>;
			};

			template<typename TMemberFunction, typename TClass, typename ...TArgs>
			constexpr static bool TIsFunctor_V = TIsFunctor<TMemberFunction, TClass, TArgs...>::bValue;
		} // Private

		template<typename>
		struct TIsMulticastDelegate : std::false_type {};

		template<typename TReturn, typename ...TArgs>
		struct TIsMulticastDelegate<TMulticastDelegate<TReturn(TArgs...)>> : std::true_type {};

		template<typename Type>
		constexpr static bool bIsMulticastDelegate_V = TIsMulticastDelegate<Type>::value;

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
		struct FContainerTypeFor<TMulticastDelegate<void(TArgs...)>>
		{
			using FType = TEventContainer<TArgs...>;
		};

		template<typename ...TArgs>
		using TContainerTypeFor = FContainerTypeFor<TArgs...>::FType;
	} // TypeTraits
	
	namespace Concepts
	{
		template<typename Type>
		concept IsMulticastDelegate = TypeTraits::bIsMulticastDelegate_V<Type>;

		template<typename TFunctor>
		concept IsFunctor =
			TypeTraits::Private::TIsFunctor_V<
				decltype(&TFunctor::operator()), TFunctor, typename ::TypeTraits::TFunctionTraits<TFunctor>::FArgsType>;
	} // Concepts
} // EventBus

/**
 * A subsystem that hold dim
 */
UCLASS()
class LOGICRAFTCOREUTILS_API UEventBusSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

	TMap<FGameplayTag, TSharedRef<IEventContainerBase>> ActiveEvents;

public:
	static ThisClass* Get(const UObject* WorldContext);
	
	template<typename TUserObject, typename TMemberFunction>
		requires std::is_base_of_v<UObject, TUserObject> &&
			std::is_member_function_pointer_v<TMemberFunction>
	FDelegateHandle AddUObject(const FGameplayTag& GameplayTag, TUserObject* UserObject, TMemberFunction Function)
	{
		return Internal_AddCallback<typename TypeTraits::TFunctionTraits<TMemberFunction>::FArgsType>(GameplayTag,
		[UserObject, Function](auto& EventContainer)
		{
			return EventContainer.MulticastDelegate.AddUObject(UserObject, Function);
		});
	}

	template<typename TUserObject, typename TMemberFunction>
		requires std::is_base_of_v<UObject, TUserObject> &&
			std::is_member_function_pointer_v<TMemberFunction>
	FDelegateHandle AddUObject(const FGameplayTag& GameplayTag, TObjectPtr<TUserObject> UserObject, TMemberFunction Function)
	{
		return AddUObject(GameplayTag, UserObject.Get(), Function);
	}

	template<typename TUserObject, typename ...TFunctionArgs>
		requires std::is_base_of_v<UObject, TUserObject>
	FDelegateHandle AddUFunction(const FGameplayTag& GameplayTag, TUserObject* UserObject, const FName& FunctionName)
	{
		return Internal_AddCallback<TFunctionArgs>(GameplayTag,
		[UserObject](auto& EventContainer, const FName& InFunctionName)
		{
			return EventContainer.MulticastDelegate.AddUFunction(UserObject, InFunctionName);
		}, FunctionName);
	}

	template<typename TUserObject, typename ...TFunctionArgs>
		requires std::is_base_of_v<UObject, TUserObject>
	FDelegateHandle AddUFunction(const FGameplayTag& GameplayTag, TObjectPtr<TUserObject> UserObject, const FName& FunctionName)
	{
		return AddUFunction<TUserObject, TFunctionArgs...>(GameplayTag, UserObject.Get(), FunctionName);
	}
	
	template<EventBus::Concepts::IsMulticastDelegate TDelegate>
	FDelegateHandle Add(const FGameplayTag& GameplayTag, TDelegate&& Delegate)
	{
		return Internal_AddCallback<TDelegate>(GameplayTag,
		[]<typename TInDelegate>(auto& EventContainer, TInDelegate&& InDelegate)
		{
			EventContainer.MulticastDelegate.Add(Forward<TInDelegate>(InDelegate));
		}, Forward<TDelegate>(Delegate));
	}

	template<typename TUserObject, EventBus::Concepts::IsFunctor TFunctor>
		requires std::is_base_of_v<UObject, TUserObject>
	FDelegateHandle AddWeakLambda(const FGameplayTag& GameplayTag, TUserObject* UserObject, TFunctor&& Functor)
	{
		return Internal_AddCallback<typename TypeTraits::TFunctionTraits<TFunctor>::FArgsType>(GameplayTag,
		[UserObject]<typename TInFunctor>(auto& EventContainer, TInFunctor&& InFunctor)
		{
			return EventContainer.MulticastDelegate.AddWeakLambda(UserObject, Forward<TInFunctor>(InFunctor));
		}, Forward<TFunctor>(Functor));
	}

	template<EventBus::Concepts::IsFunctor TFunctor>
	FDelegateHandle AddLambda(const FGameplayTag& GameplayTag, TFunctor&& Functor)
	{
		return Internal_AddCallback<typename TypeTraits::TFunctionTraits<TFunctor>::FArgsType>(GameplayTag,
		[]<typename TInFunctor>(auto& EventContainer, TInFunctor&& InFunctor)
		{
			return EventContainer.MulticastDelegate.AddLambda(Forward<TInFunctor>(InFunctor));
		}, Forward<TFunctor>(Functor));
	}

	template<typename TUserClass, typename TMemberFunction>
		requires !std::is_base_of_v<UObject, TUserClass> && std::is_member_function_pointer_v<TMemberFunction>
	FDelegateHandle AddRaw(const FGameplayTag& GameplayTag, TUserClass* UserRawObject, TMemberFunction Function)
	{
		return Internal_AddCallback<typename TypeTraits::TFunctionTraits<TMemberFunction>::FArgsType>(GameplayTag,
		[UserRawObject, Function](auto& EventContainer)
		{
			return EventContainer.MulticastDelegate.AddRaw(UserRawObject, Function);
		});
	}

	template<typename TSharedRefType, ESPMode Mode, typename TMemberFunction>
		requires std::is_member_function_pointer_v<TMemberFunction>
	FDelegateHandle AddSP(const FGameplayTag& GameplayTag, const TSharedRef<TSharedRefType, Mode>& SharedRef, TMemberFunction Function)
	{
		return Internal_AddCallback<typename TypeTraits::TFunctionTraits<TMemberFunction>::FArgsType>(GameplayTag,
		[&SharedRef, Function](auto& EventContainer)
		{
			if constexpr (Mode == ESPMode::ThreadSafe)
			{
				return EventContainer.MulticastDelegate.AddThreadSafeSP(SharedRef, Function);
			}
			else
			{
				return EventContainer.MulticastDelegate.AddSP(SharedRef, Function);
			}
		});
	}

	template<typename TSharedType, ESPMode Mode, typename TMemberFunction>
		requires std::is_base_of_v<TSharedFromThis<TSharedType, Mode>, TSharedType> &&
			std::is_member_function_pointer_v<TMemberFunction>
	FDelegateHandle AddSP(const FGameplayTag& GameplayTag, TSharedType* UserClass, TMemberFunction Function)
	{
		return AddSP(GameplayTag, StaticCastSharedRef<TSharedType, Mode>(UserClass->AsShared()), Function);
	}

	template<typename TSharedRefType, ESPMode Mode, EventBus::Concepts::IsFunctor TFunctor>
	FDelegateHandle AddSPLambda(const FGameplayTag& GameplayTag, const TSharedRef<TSharedRefType, Mode>& SharedRef, TFunctor&& Functor)
	{
		return Internal_AddCallback<typename TypeTraits::TFunctionTraits<TFunctor>::FArgsType>(GameplayTag,
		[&SharedRef]<typename TInFunctor>(auto& EventContainer, TInFunctor&& InFunctor)
		{
			return EventContainer.MulticastDelegate.AddSPLambda(SharedRef, Forward<TInFunctor>(InFunctor));
		}, Forward<TFunctor>(Functor));
	}

	template<typename TSharedType, ESPMode Mode, EventBus::Concepts::IsFunctor TFunctor>
		requires std::is_base_of_v<TSharedFromThis<TSharedType, Mode>, TSharedType>
	FDelegateHandle AddSPLambda(const FGameplayTag& GameplayTag, TSharedType* SharedRef, TFunctor&& Functor)
	{
		return Internal_AddCallback<typename TypeTraits::TFunctionTraits<TFunctor>::FArgsType>(GameplayTag,
		[&SharedRef]<typename TInFunctor>(auto& EventContainer, TInFunctor&& InFunctor)
		{
			return EventContainer.MulticastDelegate.AddSPLambda(SharedRef, Forward<TInFunctor>(InFunctor));
		}, Forward<TFunctor>(Functor));
	}

	template<typename TFunction>
		requires std::is_function_v<TFunction>
	FDelegateHandle AddStatic(const FGameplayTag& GameplayTag, TFunction Function)
	{
		return Internal_AddCallback<typename TypeTraits::TFunctionTraits<TFunction>::FArgsType>(GameplayTag,
		[Function](auto& EventContainer)
		{
			return EventContainer.MulticastDelegate.AddStatic(Function);
		});
	}

	template<typename ...TArgs>
	void Broadcast(const FGameplayTag& GameplayTag, TArgs&&... Args)
	{
		using FEventContainerType = EventBus::TypeTraits::TContainerTypeFor<TArgs...>;
		
		if (const auto BaseEventContainer{ Internal_Find(GameplayTag) })
		{
			if (ensureMsgf(BaseEventContainer->GetTypeID() == FEventContainerType::StaticGetTypeID(), TEXT("Unable to broadcast a callback of a different type.")))
			{
				auto* EventContainer = static_cast<FEventContainerType*>(&*BaseEventContainer);
				EventContainer->MulticastDelegate.Broadcast(Forward<TArgs>(Args)...);
			}
		}
	}
	
	bool Remove(const FGameplayTag& GameplayTag, FDelegateHandle DelegateHandle);
	int32 RemoveAll(const FGameplayTag& GameplayTag, const void* UserObject);

	bool IsBound(const FGameplayTag& GameplayTag) const;
	bool IsBoundToObject(const FGameplayTag& GameplayTag, const void* UserObject) const;
	
private:
	template<typename TContainerArgs, typename TCallable, typename ...TArgs>
		requires std::invocable<TCallable, EventBus::TypeTraits::TContainerTypeFor<TContainerArgs>&, TArgs...>
	FDelegateHandle Internal_AddCallback(const FGameplayTag& GameplayTag, TCallable&& Callable, TArgs&&... Args)
	{
		using FEventContainerType = EventBus::TypeTraits::TContainerTypeFor<TContainerArgs>;
		
		if (const auto BaseEventContainer{ Internal_Find(GameplayTag) })
		{
			if (ensureMsgf(BaseEventContainer->GetTypeID() == FEventContainerType::StaticGetTypeID(), TEXT("Unable to add a callback of a different type.")))
			{
				auto* EventContainer = static_cast<FEventContainerType*>(&*BaseEventContainer);
				EventContainer->AddSubscriber();
				return Callable(*EventContainer, Forward<TArgs>(Args)...);
			}
			
			return {};
		}
		
		return Callable(Internal_AddActiveEvent<FEventContainerType>(GameplayTag), Forward<TArgs>(Args)...);
	}

	template<typename TEventContainerType>
	TEventContainerType& Internal_AddActiveEvent(const FGameplayTag& GameplayTag)
	{
		auto& EventContainer{ static_cast<TEventContainerType&>(ActiveEvents.Add(GameplayTag, MakeShared<TEventContainerType>()).Get()) };
		EventContainer.AddSubscriber();
		return EventContainer;
	}

	TSharedPtr<IEventContainerBase> Internal_Find(const FGameplayTag& GameplayTag) const;

	bool Internal_Remove(const FGameplayTag& GameplayTag);
};
