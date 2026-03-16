// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#include "TestRuntimePropertyEditor.h"
#include "RuntimePropertyEditor/RuntimePropertyEditorSubsystem.h"
#include "RuntimePropertyEditor/RuntimePropertyHelper.h"

ATestRuntimePropertyEditor::ATestRuntimePropertyEditor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ATestRuntimePropertyEditor::BeginPlay()
{
	Super::BeginPlay();

	URuntimePropertyHelper::RegisterEditableObject(this, this);

	DestroySelf.Schedule([this]
	{
		Destroy();
	}, { false, 10.f });
}

void ATestRuntimePropertyEditor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	URuntimePropertyHelper::UnRegisterEditableObject(this, this);
}

void ATestRuntimePropertyEditor::OnPropertiesDisplay(FRuntimePropertyBuilder& PropertiesBuilder)
{
	PropertiesBuilder
		.AddRowProperty(
			SNew(STextBlock)
				.Text(FText::FromString("Editable Property")))
		.AddNumeric<double>(TEXT("X Value"),
				[this]{ return GetActorLocation().X; },
				[this](double NewValue)
				{
					FVector CurrentLocation{ GetActorLocation() };
					SetActorLocation({ NewValue, CurrentLocation.Y, CurrentLocation.Z });
				})
		.AddNumericVector<double, 3>(TEXT("Position"),
				[this] { return GetActorLocation(); },
				[this](const FVector& Position) { SetActorLocation(Position); })
		.AddNumericRotator<double>(TEXT("Rotation"),
				[this]{ return GetActorRotation(); },
				[this](const FRotator& Rotator){ SetActorRotation(Rotator); })
		.AddBool(TEXT("Check"),
				[this]() -> bool { return bIsEditorOnlyActor; },
				[this](bool Value){ bIsEditorOnlyActor = Value; })
		.AddSeparator(FColor::Black, 5.f)
		.AddCategory(TEXT("Category"))
		.AddNumericVector<double, 3>(TEXT("Scale"),
			[this]{ return GetActorScale3D(); },
			[this](const FVector& NewScale){ SetActorScale3D(NewScale); });
}
