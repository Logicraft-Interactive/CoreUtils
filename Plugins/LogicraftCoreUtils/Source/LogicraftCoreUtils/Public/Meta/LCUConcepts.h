// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LCUTypeTraits.h"

namespace Concept
{
	// Checks if the type is an Unreal Delegate (uses the trait defined in the other file)
	template<typename Type>
	concept IsDelegate = TypeTrait::bIsDelegate_V<Type>;

	// Checks if the type inherits from UObject (standard for UE Garbage Collection)
	template <typename Ty>
	concept DerivedFromObject = std::derived_from<Ty, UObject>;

	// Checks if the type inherits from AActor (can be placed in the world)
	template <typename Ty>
	concept DerivedFromActor = std::derived_from<Ty, AActor>;

	// Checks if TCallable can be invoked with the arguments TArgs...
	// This is a C++20 standard wrapper, very useful for validating callbacks.
	template<typename TCallable, typename ...TArgs>
	concept Invocable = std::invocable<TCallable, TArgs...>;
}