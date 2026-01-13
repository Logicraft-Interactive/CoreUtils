// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TimerHolder.h"
#include "GameFramework/Actor.h"
#include "TestTimerHolder.generated.h"

UCLASS()
class LOGICRAFTCOREUTILSSB_API ATestTimerHolder : public AActor
{
	GENERATED_BODY()

	FTimerHolder TimerHolder;
	
public:
	ATestTimerHolder();

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	void TimerFunction();
};
