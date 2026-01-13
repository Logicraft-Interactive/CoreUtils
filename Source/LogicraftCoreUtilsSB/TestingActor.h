// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TestingActor.generated.h"

UCLASS()
class LOGICRAFTCOREUTILSSB_API ATestingActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ATestingActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void TimerFunction();
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
