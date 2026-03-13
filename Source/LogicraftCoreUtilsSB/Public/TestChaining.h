// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TestChaining.generated.h"

class UCameraComponent;

UCLASS()
class LOGICRAFTCOREUTILSSB_API ATestChaining : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ATestChaining();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly)
	UCameraComponent* CameraComponent;
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
