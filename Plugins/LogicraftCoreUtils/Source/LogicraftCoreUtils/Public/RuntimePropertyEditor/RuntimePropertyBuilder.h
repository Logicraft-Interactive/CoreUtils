// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SRotatorInputBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SVectorInputBox.h"
#include "Widgets/Layout/SSeparator.h"

/**
 * 
 */
class LOGICRAFTCOREUTILS_API FRuntimePropertyBuilder
{
public:
	FRuntimePropertyBuilder() = default;
	FRuntimePropertyBuilder(const TSharedRef<SScrollBox>& InPropertiesContainer);
	~FRuntimePropertyBuilder() = default;
	
	template<typename TNumeric, typename TOnValueGet, typename TOnValueSet>
		requires
			std::is_arithmetic_v<TNumeric> &&
			std::invocable<TOnValueSet, TNumeric> &&
			std::same_as<std::invoke_result_t<TOnValueGet>, TNumeric>
	FRuntimePropertyBuilder& AddNumeric(FStringView PropertyName, TOnValueGet&& OnValueGet, TOnValueSet&& OnValueSet)
	{
		return
			AddRowProperty(PropertyName,
			SNew(SSpinBox<TNumeric>)
				.Value_Lambda(Forward<TOnValueGet>(OnValueGet))
				.OnValueChanged_Lambda(Forward<TOnValueSet>(OnValueSet)));
	}

	template<typename TNumeric, int32 NumberOfComponents, typename TOnValueGet, typename TOnValueSet>
		requires
			std::is_arithmetic_v<TNumeric> &&
			std::invocable<TOnValueSet, UE::Math::TVector<TNumeric>> &&
			std::same_as<std::invoke_result_t<TOnValueGet>, UE::Math::TVector<TNumeric>>
	FRuntimePropertyBuilder& AddNumericVector(FStringView PropertyName, TOnValueGet&& OnValueGet, TOnValueSet&& OnValueSet)
	{
		using FVectorType = UE::Math::TVector<TNumeric>;
		using SVectorWidget = SNumericVectorInputBox<TNumeric, FVectorType, NumberOfComponents>;

		#define ON_VALUE_CHANGED_COMP(Comp, ...)						 \
		On##Comp##Changed_Lambda([OnValueGet, OnValueSet](TNumeric Comp) \
		{														 		 \
			FVectorType OldVector{ OnValueGet() };			     		 \
			OnValueSet({ __VA_ARGS__ });					     		 \
		})														 		 \
		
		return
			AddRowProperty(PropertyName,
				SNew(SVectorWidget)
					.AllowSpin(true)
					.Vector_Lambda(OnValueGet)
					.PreventThrottling(true)
					.ON_VALUE_CHANGED_COMP(X, X, OldVector.Y, OldVector.Z)
					.ON_VALUE_CHANGED_COMP(Y, OldVector.X, Y, OldVector.Z)
					.ON_VALUE_CHANGED_COMP(Z, OldVector.X, OldVector.Y, Z));

		#undef ON_VALUE_CHANGED_COMP
	}

	template<typename TNumeric, typename TOnValueGet, typename TOnValueSet>
		requires
			std::is_arithmetic_v<TNumeric> &&
			std::invocable<TOnValueSet, UE::Math::TRotator<TNumeric>> &&
			std::same_as<std::invoke_result_t<TOnValueGet>, UE::Math::TRotator<TNumeric>>
	FRuntimePropertyBuilder& AddNumericRotator(FStringView PropertyName, TOnValueGet&& OnValueGet, TOnValueSet&& OnValueSet)
	{
		#define ON_VALUE_CHANGED_COMP(Comp, ...)						 \
		On##Comp##Changed_Lambda([OnValueGet, OnValueSet](TNumeric Comp) \
		{																 \
			FRotatorType OldRotator{ OnValueGet() };					 \
			OnValueSet({ __VA_ARGS__ });								 \
		})																 \

		#define ON_VALUE_SET_COMP(Comp)													   \
		Comp##_Lambda([OnValueGet]() -> TOptional<TNumeric> { return OnValueGet().Comp; }) \
		
		using FRotatorType = UE::Math::TRotator<TNumeric>;
		using SRotatorWidget = SNumericRotatorInputBox<TNumeric>;
		
		return
			AddRowProperty(PropertyName,
				SNew(SRotatorWidget)
				.AllowSpin(true)
				.PreventThrottling(true)
				.ON_VALUE_SET_COMP(Pitch)
				.ON_VALUE_SET_COMP(Yaw) 
				.ON_VALUE_SET_COMP(Roll)
				.ON_VALUE_CHANGED_COMP(Pitch, Pitch, OldRotator.Yaw, OldRotator.Roll)
				.ON_VALUE_CHANGED_COMP(Yaw, OldRotator.Pitch, Yaw, OldRotator.Roll)
				.ON_VALUE_CHANGED_COMP(Roll, OldRotator.Pitch, OldRotator.Yaw, Roll));

		#undef ON_VALUE_CHANGED_COMP
		#undef ON_VALUE_SET_COMP
	}

	template<typename TOnValueGet, typename TOnValueSet>
		requires
			std::invocable<TOnValueSet, bool> &&
			std::same_as<std::invoke_result_t<TOnValueGet>, bool>
	FRuntimePropertyBuilder& AddBool(FStringView PropertyName, TOnValueGet&& OnValueGet, TOnValueSet&& OnValueSet)
	{
		return
			AddRowProperty(PropertyName,
				SNew(SCheckBox)
				.IsChecked_Lambda([OnValueGet]
				{
					return OnValueGet() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
				})
				.OnCheckStateChanged_Lambda([OnValueSet](ECheckBoxState CheckBoxState)
				{
					OnValueSet(CheckBoxState == ECheckBoxState::Checked);
				}));
	}

	template<typename TOnClicked>
		requires
			std::invocable<TOnClicked>
	FRuntimePropertyBuilder& AddButton(FStringView PropertyName, TOnClicked&& OnClicked)
	{
		return
			AddRowProperty(
				SNew(SButton)
				.Text(FText::FromStringView(PropertyName))
				.OnClicked_Lambda([OnClicked]
				{
					OnClicked();
					return FReply::Handled();
				}));
	}

	FRuntimePropertyBuilder&  AddCategory(FStringView CategoryName);

	FRuntimePropertyBuilder& AddSeparator(const FSlateColor& ColorAndOpacity, float Thickness);

	FRuntimePropertyBuilder& AddRowProperty(const TSharedRef<SWidget>& PropertyWidget);
	FRuntimePropertyBuilder& AddRowProperty(FStringView PropertyName, const TSharedRef<SWidget>& PropertyWidget);
private:
	TSharedPtr<SScrollBox> PropertiesContainer;
};
