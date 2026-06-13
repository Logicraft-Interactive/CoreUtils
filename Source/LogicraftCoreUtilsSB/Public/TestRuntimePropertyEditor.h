// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TimerHolder.h"
#include "GameFramework/Actor.h"
#include "RuntimePropertyEditor/RuntimeEditable.h"
#include "TestRuntimePropertyEditor.generated.h"


UENUM()
enum EMyEnum
{
	Super,
	Salut,
	Prouta
};

UCLASS()
class LOGICRAFTCOREUTILSSB_API ATestRuntimePropertyEditor : public AActor, public IRuntimeEditable
{
	GENERATED_BODY()

public:
	ATestRuntimePropertyEditor();

	FTimerHolder DestroySelf;
		
	EMyEnum MyEnum;
protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void OnPropertiesDisplay(FRuntimePropertyBuilder& PropertiesBuilder) override;
};
