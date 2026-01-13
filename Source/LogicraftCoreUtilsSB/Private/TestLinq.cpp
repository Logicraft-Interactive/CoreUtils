// Fill out your copyright notice in the Description page of Project Settings.


#include "TestLinq.h"
#include "Linq.h"

// Sets default values
ATestLinq::ATestLinq()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ATestLinq::BeginPlay()
{
	Super::BeginPlay(); 

	TArray<int32> Numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

	auto Arr = Linq::Linq(Numbers).Where([](int a) {return a % 2 == 0;}).ToArray();

	 
}

// Called every frame
void ATestLinq::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

