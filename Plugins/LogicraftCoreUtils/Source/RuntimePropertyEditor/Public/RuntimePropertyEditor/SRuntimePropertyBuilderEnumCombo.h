// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LCUConcepts.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SComboBox.h"

struct FEnumData
{
	FText Tooltip{ FText::GetEmpty() };
	FName Name{ NAME_None };
	int32 Index{ 0 };
	int64 Value{ 0 };
};

/**
 * 
 */
template<typename TEnum>
	requires std::is_enum_v<TEnum>
class SRuntimePropertyBuilderEnumCombo : public SCompoundWidget
{
	DECLARE_DELEGATE_OneParam(FOnValueSet, TEnum)
	DECLARE_DELEGATE_RetVal(TEnum, FOnValueGet)
	
public:
	SLATE_BEGIN_ARGS(SRuntimePropertyBuilderEnumCombo) {}
		SLATE_EVENT(FOnValueSet, OnValueSet)
		SLATE_EVENT(FOnValueGet, OnValueGet)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		ReflectedEnum = StaticEnum<TEnum>();
		OnValueSet = InArgs._OnValueSet;
		OnValueGet = InArgs._OnValueGet;
		
		const int32 EnumValueNum{ ReflectedEnum->NumEnums() - 1 };
		
		if (EnumValueNum <= 0) return;

		for (int I = 0; I < EnumValueNum; ++I)
		{
			const int64 EnumValue{ ReflectedEnum->GetValueByIndex(I) };
			const FName EnumName{ ReflectedEnum->GetNameByIndex(I) };
			const FText EnumTooltip{ ReflectedEnum->GetToolTipTextByIndex(I) };
			
			EnumDatas.Add({ EnumTooltip, EnumName, I, EnumValue });
		}
		
		ChildSlot
		[
			SNew(SComboButton)
				.OnGetMenuContent(this, &SRuntimePropertyBuilderEnumCombo::OnGetMenuContent)
				.ButtonContent()
				[
					SNew(STextBlock)
						.Text(this, &SRuntimePropertyBuilderEnumCombo::GetCurrentEnumValue)
						.ToolTipText(this, &SRuntimePropertyBuilderEnumCombo::GetCurrentEnumTooltip)
				]
		];
	}
	
private:
	TSharedRef<SWidget> OnGetMenuContent()
	{
		constexpr bool bInShouldCloseWindowAfterMenuSelection{ true };
		constexpr bool bInCloseSelfOnly{ true };
		FMenuBuilder MenuBuilder(bInShouldCloseWindowAfterMenuSelection, nullptr, nullptr, bInCloseSelfOnly);

		for (FEnumData& EnumData : EnumDatas)
		{
			FUIAction MenuEntryAction
			{ 
				FExecuteAction::CreateLambda([this, &EnumData]
				{
					CurrentEnumData = &EnumData;
					OnValueSet.ExecuteIfBound(static_cast<TEnum>(EnumData.Value));
				}),
				FCanExecuteAction{}
			};
			
			MenuBuilder.AddMenuEntry(FText::FromName(EnumData.Name), EnumData.Tooltip, FSlateIcon{}, MoveTemp(MenuEntryAction));
		}
		
		return MenuBuilder.MakeWidget();
	}
	
	FText GetCurrentEnumValue() const
	{
		const int64 EnumValue{ static_cast<int64>(OnValueGet.Execute()) };
		verifyf((CurrentEnumData = GetEnumData(EnumValue)), TEXT("SRuntimePropertyBuilderEnumCombo: Current enum data is nullptr."));
		
		return FText::FromName(CurrentEnumData->Name);
	}
	
	FText GetCurrentEnumTooltip() const
	{
		// A copy of the code is made so verifyf crash output is accurate.
		const int64 EnumValue{ static_cast<int64>(OnValueGet.Execute()) };
		verifyf((CurrentEnumData = GetEnumData(EnumValue)), TEXT("SRuntimePropertyBuilderEnumCombo: Current enum data is nullptr."));
		
		return CurrentEnumData->Tooltip;
	}
	
	const FEnumData* GetEnumData(int64 EnumValue) const
	{
		if (CurrentEnumData && CurrentEnumData->Value == EnumValue)
		{
			return CurrentEnumData;
		}
		
		for (const FEnumData& EnumData : EnumDatas)
		{
			if (EnumData.Value == EnumValue) return &EnumData;
		}
		
		return nullptr;
	}
	
	UPROPERTY(Transient)
	UEnum* ReflectedEnum{ nullptr };
	
	TArray<FEnumData> EnumDatas;
	// Could be dangerous, but the reflected enum and array will not be updated on tick.
	mutable const FEnumData* CurrentEnumData{ nullptr }; 
	
	FOnValueSet OnValueSet;
	FOnValueGet OnValueGet;
};
