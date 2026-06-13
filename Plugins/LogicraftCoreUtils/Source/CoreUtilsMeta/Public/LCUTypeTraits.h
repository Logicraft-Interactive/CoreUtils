// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LCUConcepts.h"
#include "LCUConcepts.h"

namespace TypeTrait
{
	/**
	 * Helper to handle a variable list of template arguments (Variadic templates).
	 * Allows storing types in a TTuple and accessing a specific type by its index.
	 */
	template<typename ...TArgs>
	struct TArgsTrait
	{
		// Stores all argument types as an Unreal tuple
		using FArgs = TTuple<TArgs...>;

		// Alias to retrieve the type of the N-th argument
		template<size_t Index>
		using TArgType = TTupleElement<Index, FArgs>::Type;
	};

	/**
	 * TFunctionTraits: Main structure for analyzing function signatures.
	 * * Generic Case (Primary):
	 * Serves as a fallback for callable objects (Functors, Lambdas).
	 * Deduces the signature by inspecting the operator() of the class.
	 */
	template<typename Type>
	struct TFunctionTraits : TFunctionTraits<decltype(&Type::operator())> {};

	/**
	 * Specialization for member function pointers (e.g., &MyClass::MyFunction).
	 * Detects: Return Type, Owner Class, Arguments.
	 */
	template<typename TReturn, typename TClass, typename ...TArgs>
	struct TFunctionTraits<TReturn (TClass::*)(TArgs...)>
	{
		static constexpr bool bIsClass{ true }; // It is a class method
		static constexpr bool bIsConst{ false }; // It is not const

		using FReturnType = TReturn;
		using FClassType = TClass;
		using FArgsType = TArgsTrait<TArgs...>::FArgs;

		// Note: Use 'template' keyword here because TArgsTrait depends on template parameters.
		template<size_t Index>
		using TArgType = TArgsTrait<TArgs...>::template TArgType<Index>;

		using FFunctionType = TReturn (TClass::*)(TArgs...);
	};

	/**
	 * Specialization for CONST member methods.
	 * Identical to the previous one but handles the 'const' qualifier.
	 */
	template<typename TReturn, typename TClass, typename ...TArgs>
	struct TFunctionTraits<TReturn (TClass::*)(TArgs...) const>
	{
		static constexpr bool bIsClass{ true };
		static constexpr bool bIsConst{ true }; // Marked as const

		using FReturnType = TReturn;
		using FClassType = TClass;
		using FArgsType = TArgsTrait<TArgs...>::FArgs;

		template<size_t Index>
		using TArgType = TArgsTrait<TArgs...>::template TArgType<Index>;

		using FFunctionType = TReturn (TClass::*)(TArgs...) const;
	};

	/**
	 * Specialization for static or global functions (Raw function pointers).
	 * e.g., void MyGlobalFunc(int)
	 */
	template<typename TReturn, typename ...TArgs>
	struct TFunctionTraits<TReturn (*)(TArgs...)>
	{
		static constexpr bool bIsClass{ false }; // Not tied to a class
		static constexpr bool bIsConst{ false };

		using FReturnType = TReturn;
		using FArgsType = TArgsTrait<TArgs...>::FArgs;

		template<size_t Index>
		using TArgType = TArgsTrait<TArgs...>::template TArgType<Index>;

		using FFunctionType = TReturn (*)(TArgs...);
	};

	/**
	 * Traits to detect if a type is an Unreal Engine TDelegate.
	 * Uses inheritance from std::true_type / std::false_type.
	 */
	template<typename>
	struct TIsDelegate : std::false_type {};

	// Specialization: If the type matches the TDelegate<Signature> pattern, it is true.
	template<typename TReturn, typename ...TArgs>
	struct TIsDelegate<TDelegate<TReturn(TArgs...)>> : std::true_type {};

	// Helper variable to simplify writing (e.g., if (bIsDelegate_V<MyType>))
	template<typename Type>
	constexpr bool bIsDelegate_V = TIsDelegate<std::remove_cvref_t<Type>>::value;

	/**
	 * Very powerful trait to check if a type T is an instantiation of a specific Template.
	 * Usage example: Is 'MyVar' a 'TArray<...>' regardless of what it contains?
	 */
	template <template <typename...> class Container, typename Args>
	struct TIsInstantiation : std::false_type {};

	// Specialization that matches if the second argument is indeed Container<Args...>
	template <template<typename...> class Container ,typename... Args>
	struct TIsInstantiation<Container,Container<Args...>> : std::true_type {};

	template <template<typename...> class Container ,typename T>
	constexpr bool bIsInstantiation_V = TIsInstantiation<Container, std::remove_cvref_t<T>>::value;

	/**
	 * Traits to uniformly extract the value type contained in a container.
	 * Useful because TArray uses ElementType, but TMap is more complex (Pairs).
	 */
	template <typename C, typename = void>
	struct TContainerValueType
	{
		// Default case (TArray, TSet): take ElementType
		using Type = typename C::ElementType;
	};

	// Specialization for TMap: The value type is a TPair<Key, Value>
	template <typename... Args>
	struct TContainerValueType<TMap<Args...>>
	{
		using Type = TPair<typename TMap<Args...>::KeyType, typename TMap<Args...>::ValueType>;
	};

	// Helper alias
	template <typename C>
	using TContainerValueType_T = typename TContainerValueType<std::remove_reference_t<C>>::Type;
} // TypeTraits