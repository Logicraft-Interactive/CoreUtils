// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "EventBusSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE(TestDelegate)

namespace TypeTraits
{
	template<typename>
	struct TIsMulticastDelegate : std::false_type {};

	template<typename TReturn, typename ...TArgs>
	struct TIsMulticastDelegate<TMulticastDelegate<TReturn(TArgs...)>> : std::true_type {};

	template<typename Type>
	constexpr static bool bIsMulticastDelegate_V = TIsMulticastDelegate<Type>::value;

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

	template<typename, typename...>
	struct TIsInvocableWithTupleArgs
	{
		using FFunctionType = void;
		constexpr static bool bValue = false;
	};

	template<typename TReturn, typename ...TArgs>
	struct TIsInvocableWithTupleArgs<TReturn (*)(TArgs...), TTuple<TArgs...>>
	{
		using FFunctionType = TReturn (*)(TArgs...);
		constexpr static bool bValue = std::is_invocable_v<FFunctionType, TArgs...>;
	};

	template<typename TReturn, typename TClass, typename ...TArgs>
	struct TIsInvocableWithTupleArgs<TReturn (TClass::*)(TArgs...), TClass, TTuple<TArgs...>>
	{
		using FFunctionType = TReturn (TClass::*)(TArgs...);
		constexpr static bool bValue = std::is_invocable_v<FFunctionType, TClass, TArgs...>;
	};

	template<typename TReturn, typename TClass, typename ...TArgs>
	struct TIsInvocableWithTupleArgs<TReturn (TClass::*)(TArgs...) const, TClass, TTuple<TArgs...>>
	{
		using FFunctionType = TReturn (TClass::*)(TArgs...) const;
		constexpr static bool bValue = std::is_invocable_v<FFunctionType, TClass, TArgs...>;
	};

	template<typename Type, typename ...TArgs>
	constexpr static bool TIsInvocableWithTupleArgs_V = TIsInvocableWithTupleArgs<Type, TArgs...>::bValue;
} // TypeTraits

namespace Concepts
{
	template<typename Type>
	concept IsMulticastDelegate = TypeTraits::bIsMulticastDelegate_V<Type>;

	template<typename Type, typename ...TArgs>
	concept IsInvocableWithTupleArgs = TypeTraits::TIsInvocableWithTupleArgs_V<Type, TArgs...>;
} // Concepts

struct IEventContainerBase
{
	IEventContainerBase() = default;
	virtual ~IEventContainerBase() = default;

	virtual const void* GetTypeID() = 0;
};

template<typename ...Args>
struct TEventContainer : IEventContainerBase
{
private:
	static const uint32 TypeId{ 0U };
	
public:
	TMulticastDelegate<void(Args...)> MulticastDelegate;
	
	virtual const void* GetTypeID() override { return &TypeId; }
	static const void* StaticGetTypeID() { return &TypeId; }
};

namespace TypeTraits
{
	template<typename>
	struct FContainerTypeFor;

	template<typename ...TArgs>
	struct FContainerTypeFor<TArgs...>
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

	template<typename Ty>
	using TContainerTypeFor = FContainerTypeFor<Ty>::FType;
}

/**
 * 
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
			Concepts::IsInvocableWithTupleArgs<TMemberFunction, TUserObject, typename TypeTraits::TFunctionTraits<TMemberFunction>::FArgsType>
	FDelegateHandle AddUObject(const FGameplayTag& GameplayTag, TUserObject* UserObject, TMemberFunction Function)
	{
		return Internal_Add<typename TypeTraits::TFunctionTraits<TMemberFunction>::FArgsType>(GameplayTag,
		[UserObject, Function](auto& EventContainer)
		{
			return EventContainer.MulticastDelegate.AddUObject(UserObject, Function);
		});
	}

	template<typename TUserObject, typename TMemberFunction>
	requires std::is_base_of_v<UObject, TUserObject> &&
			Concepts::IsInvocableWithTupleArgs<TMemberFunction, TUserObject, typename TypeTraits::TFunctionTraits<TMemberFunction>::FArgsType>
	FDelegateHandle AddUObject(const FGameplayTag& GameplayTag, TObjectPtr<TUserObject> UserObject, TMemberFunction Function)
	{
		return AddUObject(GameplayTag, UserObject.Get(), Function);
	}

	template<typename TUserObject, typename ...TFunctionArgs>
		requires std::is_base_of_v<UObject, TUserObject>
	FDelegateHandle AddUFunction(const FGameplayTag& GameplayTag, TUserObject* UserObject, const FName& FunctionName)
	{
		return Internal_Add<TFunctionArgs>(GameplayTag,
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
	
	template<Concepts::IsMulticastDelegate TDelegate>
	FDelegateHandle Add(const FGameplayTag& GameplayTag, TDelegate&& Delegate)
	{
		return Internal_Add<TDelegate>(GameplayTag,
		[]<typename TInDelegate>(auto& EventContainer, TInDelegate&& InDelegate)
		{
			EventContainer.MulticastDelegate.Add(Forward<TInDelegate>(InDelegate));
		}, Forward<TDelegate>(Delegate));
	}

	template<typename TUserObject, typename TFunctor>
		requires std::is_base_of_v<UObject, TUserObject> &&
			Concepts::IsInvocableWithTupleArgs<decltype(&TFunctor::operator()), TFunctor, typename TypeTraits::TFunctionTraits<TFunctor>::FArgsType>
	FDelegateHandle AddWeakLambda(const FGameplayTag& GameplayTag, TUserObject* UserObject, TFunctor&& Functor)
	{
		return Internal_Add<TypeTraits::TFunctionTraits<TFunctor>::FArgsType>(GameplayTag,
		[UserObject]<typename TInFunctor>(auto& EventContainer, TInFunctor&& InFunctor)
		{
			return EventContainer.MulticastDelegate.AddWeakLambda(UserObject, Forward<TInFunctor>(InFunctor));
		}, Forward<TFunctor>(Functor));
	}

	template<typename TFunctor>
		requires Concepts::IsInvocableWithTupleArgs<decltype(&TFunctor::operator()), TFunctor, typename TypeTraits::TFunctionTraits<TFunctor>::FArgsType>
	FDelegateHandle AddLambda(const FGameplayTag& GameplayTag, TFunctor&& Functor)
	{
		return Internal_Add<typename TypeTraits::TFunctionTraits<TFunctor>::FArgsType>(GameplayTag,
		[]<typename TInFunctor>(auto& EventContainer, TInFunctor&& InFunctor)
		{
			return EventContainer.MulticastDelegate.AddLambda(Forward<TInFunctor>(InFunctor));
		}, Forward<TFunctor>(Functor));
	}

	template<typename TUserClass, typename TMemberFunction>
		requires !std::is_base_of_v<UObject, TUserClass> &&
			Concepts::IsInvocableWithTupleArgs<TMemberFunction, TUserClass, typename TypeTraits::TFunctionTraits<TMemberFunction>::FArgsType>
	FDelegateHandle AddRaw(const FGameplayTag& GameplayTag, TUserClass* UserRawObject, TMemberFunction Function)
	{
		return Internal_Add<TypeTraits::TFunctionTraits<TMemberFunction>::FArgsType>(GameplayTag,
		[UserRawObject, Function](auto& EventContainer)
		{
			return EventContainer.MulticastDelegate.AddRaw(UserRawObject, Function);
		});
	}

	template<typename TSharedRefType, ESPMode Mode, typename TMemberFunction>
		requires Concepts::IsInvocableWithTupleArgs<TMemberFunction, TSharedRefType, typename TypeTraits::TFunctionTraits<TMemberFunction>::FArgsType>
	FDelegateHandle AddSP(const FGameplayTag& GameplayTag, const TSharedRef<TSharedRefType, Mode>& SharedRef, TMemberFunction Function)
	{
		return Internal_Add<TypeTraits::TFunctionTraits<TMemberFunction>::FArgsType>(GameplayTag,
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
			Concepts::IsInvocableWithTupleArgs<TMemberFunction, TSharedType, typename TypeTraits::TFunctionTraits<TMemberFunction>::FArgsType>
	FDelegateHandle AddSP(const FGameplayTag& GameplayTag, TSharedType* UserClass, TMemberFunction Function)
	{
		return AddSP(GameplayTag, StaticCastSharedRef<TSharedType, Mode>(UserClass->AsShared()), Function);
	}

	template<typename TSharedRefType, ESPMode Mode, typename TFunctor>
		requires Concepts::IsInvocableWithTupleArgs<decltype(&TFunctor::operator()), TFunctor, typename TypeTraits::TFunctionTraits<TFunctor>::FArgsType>
	FDelegateHandle AddSPLambda(const FGameplayTag& GameplayTag, const TSharedRef<TSharedRefType, Mode>& SharedRef, TFunctor&& Functor)
	{
		return Internal_Add<TypeTraits::TFunctionTraits<TFunctor>::FArgsType>(GameplayTag,
		[&SharedRef]<typename TInFunctor>(auto& EventContainer, TInFunctor&& InFunctor)
		{
			return EventContainer.MulticastDelegate.AddSPLambda(SharedRef, Forward<TInFunctor>(InFunctor));
		}, Forward<TFunctor>(Functor));
	}

	template<typename TSharedType, ESPMode Mode, typename TFunctor>
		requires std::is_base_of_v<TSharedFromThis<TSharedType, Mode>, TSharedType> &&
			Concepts::IsInvocableWithTupleArgs<decltype(&TFunctor::operator()), TFunctor, typename TypeTraits::TFunctionTraits<TFunctor>::FArgsType>
	FDelegateHandle AddSPLambda(const FGameplayTag& GameplayTag, TSharedType* SharedRef, TFunctor&& Functor)
	{
		return Internal_Add<TypeTraits::TFunctionTraits<TFunctor>::FArgsType>(GameplayTag,
		[&SharedRef]<typename TInFunctor>(auto& EventContainer, TInFunctor&& InFunctor)
		{
			return EventContainer.MulticastDelegate.AddSPLambda(SharedRef, Forward<TInFunctor>(InFunctor));
		}, Forward<TFunctor>(Functor));
	}

	template<typename TFunction>
		requires Concepts::IsInvocableWithTupleArgs<TFunction, typename TypeTraits::TFunctionTraits<TFunction>::FArgsType>
	FDelegateHandle AddStatic(const FGameplayTag& GameplayTag, TFunction Function)
	{
		return Internal_Add<TypeTraits::TFunctionTraits<TFunction>::FArgsType>(GameplayTag,
		[Function](auto& EventContainer)
		{
			return EventContainer.MulticastDelegate.AddStatic(Function);
		});
	}

	template<typename ...TArgs>
	void Broadcast(const FGameplayTag& GameplayTag, TArgs&&... Args)
	{
		using FEventContainerType = TypeTraits::TContainerTypeFor<TArgs>;
		
		if (TSharedRef<IEventContainerBase>* BaseEventContainerPtr{ ActiveEvents.Find(GameplayTag) })
		{
			auto& BaseEventContainerRef{ *BaseEventContainerPtr };
			if (ensureMsgf(BaseEventContainerRef->GetTypeID() == FEventContainerType::StaticGetTypeID(), TEXT("Unable to broadcast a callback of a different type.")))
			{
				auto* EventContainer = static_cast<FEventContainerType*>(&*BaseEventContainerRef);
				EventContainer->MulticastDelegate.Broadcast(Forward<TArgs>(Args)...);
			}
		}
	}
	
private:
	template<typename TContainerArgs, typename TCallable, typename ...TArgs>
		requires std::invocable<TCallable, TypeTraits::TContainerTypeFor<TContainerArgs>&, TArgs...>
	FDelegateHandle Internal_Add(const FGameplayTag& GameplayTag, TCallable&& Callable, TArgs&&... Args)
	{
		using FEventContainerType = TypeTraits::TContainerTypeFor<TContainerArgs>;
		
		if (TSharedRef<IEventContainerBase>* BaseEventContainerPtr{ ActiveEvents.Find(GameplayTag) })
		{
			auto& BaseEventContainerRef{ *BaseEventContainerPtr };
			if (ensureMsgf(BaseEventContainerRef->GetTypeID() == FEventContainerType::StaticGetTypeID(), TEXT("Unable to add a callback of a different type.")))
			{
				auto* EventContainer = static_cast<FEventContainerType*>(&*BaseEventContainerRef);
				return Callable(*EventContainer, Forward<TFunction>(Args)...);
			}
			
			return {};
		}

		auto* EventContainer = static_cast<FEventContainerType*>(&*ActiveEvents.Add(GameplayTag, MakeShared<FEventContainerType>()));
		return Callable(*EventContainer, Forward<TFunction>(Args)...);
	}
};
