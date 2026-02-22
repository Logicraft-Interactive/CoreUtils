// Copyright (c) Logicraft Interactive. All Rights Reserved.


#include "TestRuntimePropertyEditor.h"
#include "RuntimePropertyEditor/RuntimePropertyEditorSubsystem.h"
#include "Widgets/Input/SVectorInputBox.h"

ATestRuntimePropertyEditor::ATestRuntimePropertyEditor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ATestRuntimePropertyEditor::BeginPlay()
{
	Super::BeginPlay();

	URuntimePropertyEditorSubsystem::Get(this)->RegisterEditableProperties(this);
}

void ATestRuntimePropertyEditor::OnPropertiesDisplay(FRuntimePropertyBuilder& PropertiesBuilder)
{
	PropertiesBuilder
		.AddRowProperty(
			SNew(STextBlock)
				.Text(FText::FromString("Editable Property")))
		.AddNumeric<double>(TEXT("X Value"),
				[this]{ return GetActorLocation().X; },
				[this](float NewValue)
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
		.AddCategory(TEXT("Category"));
}