// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TimerHolder.h"
#include "GameFramework/Actor.h"
#include "TestTimerHolder.generated.h"

UCLASS()
class LOGICRAFTCOREUTILSSB_API ATestTimerHolder : public AActor
{
	GENERATED_BODY()

	FTimerHolder LambdaTimerHolder;
	FTimerHolder MemberTimerHolder;
	
public:
	ATestTimerHolder();

protected:
	virtual void BeginPlay() override;
	
	void TimerFunction();
};
