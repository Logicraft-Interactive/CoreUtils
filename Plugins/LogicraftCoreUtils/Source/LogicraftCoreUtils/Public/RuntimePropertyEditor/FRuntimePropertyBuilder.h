// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Input/SSpinBox.h"

/**
 * 
 */
class LOGICRAFTCOREUTILS_API FRuntimePropertyBuilder
{
public:
	FRuntimePropertyBuilder() = default;
	FRuntimePropertyBuilder(const TSharedRef<SScrollBox>& InPropertiesContainer);
	~FRuntimePropertyBuilder() = default;

	//Vector, Rotator.
	//Numeric types.
	//Bool
	//Button
	//Separator
	//Category

	template<typename TNumeric, typename TOnValueGet, typename TOnValueSet>
		requires
			std::invocable<TOnValueSet, TNumeric> &&
			requires(TOnValueGet OnValueGet)
			{
				{ OnValueGet() } -> std::same_as<TNumeric>;
			}
	FRuntimePropertyBuilder& AddNumeric(FStringView PropertyName, TOnValueGet&& OnValueGet, TOnValueSet&& OnValueSet)
	{
		return
			AddRowProperty(PropertyName,
			SNew(SSpinBox<TNumeric>)
				.Value_Lambda(Forward<TOnValueGet>(OnValueGet))
				.OnValueChanged_Lambda(Forward<TOnValueSet>(OnValueSet)));
	}

	FRuntimePropertyBuilder& AddRowProperty(FStringView PropertyName, const TSharedRef<SWidget>& PropertyWidget);

	TSharedRef<SScrollBox> GetPropertiesContainer() const;
private:
	TSharedPtr<SScrollBox> PropertiesContainer;
};
