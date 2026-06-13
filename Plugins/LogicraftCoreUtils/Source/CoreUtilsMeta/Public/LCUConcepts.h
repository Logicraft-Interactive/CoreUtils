// CORE UTILS - PLUGIN LICENSE
//
// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.
// 
// 1. PERMISSIONS
// - You are granted a non-exclusive, worldwide, royalty-free license to use, copy, and modify "Core Utils" (the "Plugin").
// - You may use the Plugin in any personal or commercial projects (e.g., video games, simulations, interactive applications).
// - You may redistribute the Plugin as part of a compiled, standalone software product (shipped game or application).
// 
// 2. RESTRICTIONS
// - You are STRICTLY PROHIBITED from selling, licensing, or sub-licensing the Plugin source code, binaries, or assets as a standalone product or as part of a toolset, even if modifications have been made.
// - You may not redistribute the source code of this Plugin in a public repository unless it is a fork that maintains this original license.
// 
// 3. ATTRIBUTION
// - Attribution is MANDATORY. You must include a credit to "Logicraft Interactive" in your project's credits, "About" section, or documentation (e.g., "Includes Core Utils by Logicraft Interactive").
// 
// 4. DISCLAIMER
// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LCUTypeTraits.h"
#include "GameFramework/Actor.h"

namespace Concept
{
	// Checks if the type is an Unreal Delegate (uses the trait defined in the other file)
	template<typename Type>
	concept IsDelegate = TypeTrait::bIsDelegate_V<Type>;

	// Checks if the type inherits from UObject (standard for UE Garbage Collection)
	template <typename Ty>
	concept DerivedFromObject = std::derived_from<Ty, UObject>
	|| std::is_convertible_v<Ty, UObject*>;
	

	// Checks if the type inherits from AActor (can be placed in the world)
	template <typename Ty>
	concept DerivedFromActor = std::derived_from<Ty, AActor>
	|| std::is_convertible_v<Ty, AActor*>;

	// Checks if TCallable can be invoked with the arguments TArgs...
	// This is a C++20 standard wrapper, very useful for validating callbacks.
	template<typename TCallable, typename ...TArgs>
	concept Invocable = std::invocable<TCallable, TArgs...>;
}