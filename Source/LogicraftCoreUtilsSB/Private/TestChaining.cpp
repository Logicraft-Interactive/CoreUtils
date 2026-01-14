// Fill out your copyright notice in the Description page of Project Settings.


#include "LogicraftCoreUtilsSB/Public/TestChaining.h"
#include "Chain.h"
#include "Camera/CameraComponent.h"

// Sets default values
ATestChaining::ATestChaining()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>("Root");
	
	CameraComponent = CreateDefaultSubobject<UCameraComponent>("Camera Component");
	if (RootComponent)
	{
		CameraComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	}
}

// Called when the game starts or when spawned
void ATestChaining::BeginPlay()
{
	Super::BeginPlay();

	auto Name = Chain::StartChain(CameraComponent)
	.Transform(&UCameraComponent::GetAttachmentRoot)
	.Execute([](USceneComponent* SceneComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("Passing into Execute from Chain"));
		SceneComponent->bHiddenInGame = false;
	})
	.Cast<UObject>()
	.GetValue([](const UObject* Object)
	{
		return Object->GetName();
	});

	Chain::Execute(CameraComponent, [](const UCameraComponent* Camera)
	{
		UE_LOG(LogTemp, Error, TEXT("Passing into Execute standalone"));
		Camera->AspectRatio;
	});
	USceneComponent* A = Chain::Transform(CameraComponent, [](const UCameraComponent* Camera)
	{		
		UE_LOG(LogTemp, Error, TEXT("Passing into Transform standalone"));
		return Camera->GetAttachmentRoot();
	});
}

// Called every frame
void ATestChaining::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime); 
}

