// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Meta/LCUConcepts.h"

struct LOGICRAFTCOREUTILS_API FLCUHelper
{

	template<Concept::DerivedFromObject ObjectType, typename InstantiateObject = std::remove_pointer_t<ObjectType>>
	static void CreateDefaultSubobject(ObjectType& Object, const FName& ObjectName, USceneComponent* Parent = nullptr, bool bTransient = false)
	{
		Object = Object->template CreateDefaultSubobject<InstantiateObject>(ObjectName, bTransient);
		if (Parent)
		{
			Object->SetupAttachment(Parent);
		}
	}
};
