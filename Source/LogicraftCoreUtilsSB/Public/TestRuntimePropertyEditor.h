// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RuntimePropertyEditor/RuntimeEditable.h"
#include "TestRuntimePropertyEditor.generated.h"

UCLASS()
class LOGICRAFTCOREUTILSSB_API ATestRuntimePropertyEditor : public AActor, public IRuntimeEditable
{
	GENERATED_BODY()

public:
	ATestRuntimePropertyEditor();

protected:
	virtual void BeginPlay() override;

	virtual void OnPropertiesDisplay(FRuntimePropertyBuilder PropertiesBuilder) override;
};
