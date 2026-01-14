// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "EventBusSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE(TestDelegate)

namespace TypeTraits
{
	template<typename Type>
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

	// template<typename TFunction, typename ...TArgs>
	// struct TIsInvocableNoArgs
	// {
	// 	static constexpr bool bValue{ false };
	// };
	//
	// template<typename TReturn, typename ...TArgs>
	// struct TIsInvocableNoArgs<TReturn (*)(TArgs...), TArgs...>
	// {
	// 	using FFunctionType = TReturn (*)(TArgs...);
	// 	
	// 	static constexpr bool bValue{ std::is_invocable_v<FFunctionType, TArgs...> };
	// };
	//
	// template<typename TReturn, typename TClass, typename ...TArgs>
	// struct TIsInvocableNoArgs<TReturn (TClass::*)(TArgs...), TClass, TArgs...>
	// {
	// 	using FFunctionType = TReturn (TClass::*)(TArgs...);
	// 	
	// 	static constexpr bool bValue{ std::is_invocable_v<FFunctionType, TClass, TArgs...> };
	// };
	//
	// template<typename TReturn, typename TClass, typename ...TArgs>
	// struct TIsInvocableNoArgs<TReturn (TClass::*)(TArgs...) const, TClass, TArgs...>
	// {
	// 	using FFunctionType = TReturn (TClass::*)(TArgs...) const;
	// 	
	// 	static constexpr bool bValue{ std::is_invocable_v<FFunctionType, TClass, TArgs...> };
	// };
	//
	// template<typename TFunction, typename ...TArgs>
	// constexpr static bool bIsInvocableNoArgs{ TIsInvocableNoArgs<TFunction, TArgs...>::bValue };
} // TypeTraits

namespace Concepts
{
	template<typename Type>
	concept IsMulticastDelegate = TypeTraits::bIsMulticastDelegate_V<Type>;

	// template<typename TFunction, typename ...TArgs>
	// concept InvocableNoArgs = TypeTraits::bIsInvocableNoArgs<TFunction, TArgs...>;
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

template<typename Type>
struct FContainerTypeFor;

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
	
	template<typename TUserObject, typename TMemberFunction, typename ...TArgs>
		requires
			(std::invocable<TMemberFunction, TUserObject> || std::same_as<TMemberFunction, FName>) &&
				std::is_base_of_v<UObject, TUserObject>
	FDelegateHandle AddUObject(const FGameplayTag& GameplayTag, TUserObject* UserObject, TMemberFunction&& Function)
	{
		return Internal_Add<TypeTraits::TFunctionTraits<TMemberFunction>::FArgsType>(GameplayTag,
		[UserObject, InFunction = Forward<TMemberFunction>(Function)](auto& EventContainer)
		{
			if (std::same_as<TMemberFunction, FName>)
			{
				return EventContainer.MulticastDelegate.AddUFunction(UserObject, Forward<TMemberFunction>(InFunction));
			}
			else
			{
				return EventContainer.MulticastDelegate.AddUObject(UserObject, Forward<TMemberFunction>(InFunction));
			}
		});
	}

	template<typename TUserObject, typename TMemberFunction>
	requires
		(std::invocable<TMemberFunction, TUserObject> || std::same_as<TMemberFunction, FName>) &&
			std::is_base_of_v<UObject, TUserObject>
	FDelegateHandle AddUObject(const FGameplayTag& GameplayTag, TObjectPtr<TUserObject> UserObject, TMemberFunction&& Function)
	{
		return AddUObject(GameplayTag, UserObject.Get(), Forward<TMemberFunction>(Function));
	}

	template<Concepts::IsMulticastDelegate TDelegate>
	FDelegateHandle Add(const FGameplayTag& GameplayTag, TDelegate&& Delegate)
	{
		return Internal_Add<TDelegate>(GameplayTag,
		[InDelegate = Forward<TDelegate>(Delegate)](auto& EventContainer)
		{
			EventContainer->MulticastDelegate.Add(Forward<TDelegate>(InDelegate));
		});
	}

	template<typename TUserObject, std::invocable TFunctor>
		requires std::is_base_of_v<UObject, TUserObject>
	FDelegateHandle AddWeakLambda(const FGameplayTag& GameplayTag, TUserObject* UserObject, TFunctor&& Functor)
	{
		return Internal_Add<TypeTraits::TFunctionTraits<TFunctor>::FArgsType>(GameplayTag,
		[UserObject, InFunctor = Forward<TFunctor>(Functor)](auto& EventContainer)
		{
			return EventContainer->MulticastDelegate.AddWeakLambda(UserObject, Forward<TFunctor>(InFunctor));
		});
	}

	template<std::invocable TFunctor>
	FDelegateHandle AddLambda(const FGameplayTag& GameplayTag, TFunctor&& Functor)
	{
		return Internal_Add<TypeTraits::TFunctionTraits<TFunctor>::FArgsType>(GameplayTag,
		[InFunctor = Forward<TFunctor>(Functor)](auto& EventContainer)
		{
			return EventContainer->MulticastDelegate.AddLambda(Forward<TFunctor>(InFunctor));
		});
	}

	template<typename TUserClass, typename TMemberFunction>
		requires std::invocable<TMemberFunction, TUserClass> && !std::is_base_of_v<UObject, TUserClass>
	FDelegateHandle AddRaw(const FGameplayTag& GameplayTag, TUserClass* UserRawObject, TMemberFunction&& Function)
	{
		return Internal_Add<TypeTraits::TFunctionTraits<TMemberFunction>::FArgsType>(GameplayTag,
		[UserRawObject, InFunction = Forward<TMemberFunction>(Function)](auto& EventContainer)
		{
			return EventContainer->MulticastDelegate.AddRaw(UserRawObject, Forward<TMemberFunction>(InFunction));
		});
	}

	template<typename TSharedRefType, ESPMode Mode, typename TMemberFunction>
		requires std::invocable<TMemberFunction, TSharedRefType>
	FDelegateHandle AddSP(const FGameplayTag& GameplayTag, const TSharedRef<TSharedRefType, Mode>& SharedRef, TMemberFunction&& Function)
	{
		return Internal_Add<TypeTraits::TFunctionTraits<TMemberFunction>::FArgsType>(GameplayTag,
		[&SharedRef, InFunction = Forward<TMemberFunction>(Function)](auto& EventContainer)
		{
			if constexpr (Mode == ESPMode::ThreadSafe)
			{
				return EventContainer.MulticastDelegate.AddThreadSafeSP(SharedRef, Forward<TMemberFunction>(InFunction));
			}
			else
			{
				return EventContainer.MulticastDelegate.AddSP(SharedRef, Forward<TMemberFunction>(InFunction));
			}
		});
	}

	template<typename TSharedType, ESPMode Mode, typename TMemberFunction>
		requires std::is_base_of_v<TSharedFromThis<TSharedType, Mode>, TSharedType> && std::invocable<TMemberFunction, TSharedType>
	FDelegateHandle AddSP(const FGameplayTag& GameplayTag, TSharedType* UserClass, TMemberFunction&& Function)
	{
		return AddSP(GameplayTag, StaticCastSharedRef<TSharedType, Mode>(UserClass->AsShared()), Forward<TMemberFunction>(Function));
	}

	template<typename TSharedRefType, ESPMode Mode, typename TFunctor>
		requires std::invocable<TFunctor>
	FDelegateHandle AddSPLambda(const FGameplayTag& GameplayTag, const TSharedRef<TSharedRefType, Mode>& SharedRef, TFunctor&& Functor)
	{
		return Internal_Add<TypeTraits::TFunctionTraits<TFunctor>::FArgsType>(GameplayTag,
		[&SharedRef, InFunctor = Forward<TFunctor>(Functor)](auto& EventContainer)
		{
			return EventContainer->MulticastDelegate.AddSPLambda(SharedRef, Forward<TFunctor>(InFunctor));
		});
	}

	template<typename TSharedType, ESPMode Mode, typename TFunctor>
		requires std::is_base_of_v<TSharedFromThis<TSharedType, Mode>, TSharedType> && std::invocable<TFunctor>
	FDelegateHandle AddSPLambda(const FGameplayTag& GameplayTag, TSharedType* SharedRef, TFunctor&& Functor)
	{
		return Internal_Add<TypeTraits::TFunctionTraits<TFunctor>::FArgsType>(GameplayTag,
		[&SharedRef, InFunctor = Forward<TFunctor>(Functor)](auto& EventContainer)
		{
			return EventContainer->MulticastDelegate.AddSPLambda(SharedRef, Forward<TFunctor>(InFunctor));
		});
	}

	template<std::invocable TFunction>
	FDelegateHandle AddStatic(const FGameplayTag& GameplayTag, TFunction&& Function)
	{
		return Internal_Add<TypeTraits::TFunctionTraits<TFunction>::FArgsType>(GameplayTag,
		[InFunction = Forward<TFunction>(Function)](auto& EventContainer)
		{
			return EventContainer->MulticastDelegate.AddStatic(Forward<TFunction>(InFunction));
		});
	}
	
private:
	template<typename TContainerArgs>
	FDelegateHandle Internal_Add(const FGameplayTag& GameplayTag, std::invocable auto&& Callable)
	{
		using FEventContainerType = TContainerTypeFor<TContainerArgs>;

		if (TSharedRef<IEventContainerBase>* BaseEventContainerPtr{ ActiveEvents.Find(GameplayTag) })
		{
			auto& BaseEventContainerRef{ *BaseEventContainerPtr };
			if (ensureMsgf(BaseEventContainerRef->GetTypeID() == FEventContainerType::StaticGetTypeID, TEXT("Unable to add a callback of a different type.")))
			{
				auto* EventContainer = static_cast<FEventContainerType*>(&*BaseEventContainerRef);
				return Callable(*EventContainer);
			}
			
			return {};
		}

		return Callable(*ActiveEvents.Add(GameplayTag, MakeShared<FEventContainerType>()));
	}
};
