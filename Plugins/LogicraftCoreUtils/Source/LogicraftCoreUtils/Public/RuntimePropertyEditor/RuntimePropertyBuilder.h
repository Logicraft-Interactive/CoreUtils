// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SRotatorInputBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SVectorInputBox.h"
#include "Widgets/Layout/SSeparator.h"

class SScrollBox;

/**
 * Fluent API builder for constructing runtime property editor UIs.
 * 
 * This class provides a chainable interface to add various types of properties,
 * buttons, and UI elements to the runtime property editor without requiring
 * detailed knowledge of Slate widgets.
 * 
 * The builder abstracts away the complexity of Slate widget construction and
 * provides a simple, type-safe API for creating property editors.
 * 
 * @note All Add* methods return a reference to *this, allowing method chaining.
 * 
 * Thread Safety: Not thread-safe. Should only be used on the Game Thread.
 */
class LOGICRAFTCOREUTILS_API FRuntimePropertyBuilder
{
public:
	/** Default constructor. Creates an uninitialized builder (should not be used directly). */
	FRuntimePropertyBuilder() = default;
	
	/**
	 * Constructs a property builder that adds widgets to the specified container.
	 * 
	 * @param InPropertiesContainer - The Slate scroll box that will contain all added properties.
	 */
	FRuntimePropertyBuilder(const TSharedRef<SScrollBox>& InPropertiesContainer);
	
	/** Destructor */
	~FRuntimePropertyBuilder() = default;
	
	/**
	 * Adds a numeric property editor (int, float, double, etc.).
	 * 
	 * Creates a spin box widget that allows the user to modify numeric values
	 * either by typing or using mouse wheel / arrow buttons.
	 * 
	 * @tparam TNumeric - Numeric type (int32, float, double, etc.)
	 * @tparam TOnValueGet - Callable type that returns TNumeric (lambda, function pointer, etc.)
	 * @tparam TOnValueSet - Callable type that accepts TNumeric (lambda, function pointer, etc.)
	 * 
	 * @param PropertyName - Display name shown to the left of the widget
	 * @param OnValueGet - Getter lambda/function called to retrieve the current value
	 * @param OnValueSet - Setter lambda/function called when the user modifies the value
	 * 
	 * @return Reference to this builder for method chaining
	 * 
	 * Example:
	 * @code
	 * Builder.AddNumeric<float>("Health",
	 *     [this]() { return Health; },
	 *     [this](float Val) { Health = Val; });
	 * @endcode
	 */
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

	/**
	 * Adds a vector property editor (FVector, FVector2D, etc.).
	 * 
	 * Creates a multi-component numeric input box with X, Y, Z fields (or fewer for lower-dimensional vectors).
	 * Supports real-time editing with throttling to prevent network spam.
	 * 
	 * @tparam TNumeric - Component type (float, double)
	 * @tparam NumberOfComponents - Number of vector components (2 for FVector2D, 3 for FVector)
	 * @tparam TOnValueGet - Callable returning TVector<TNumeric>
	 * @tparam TOnValueSet - Callable accepting TVector<TNumeric>
	 * 
	 * @param PropertyName - Display name
	 * @param OnValueGet - Getter for current vector value
	 * @param OnValueSet - Setter called when any component changes
	 * 
	 * @return Reference to this builder for chaining
	 * 
	 * Example:
	 * @code
	 * Builder.AddNumericVector<double, 3>("Position",
	 *     [this]() { return GetActorLocation(); },
	 *     [this](FVector Pos) { SetActorLocation(Pos); });
	 * @endcode
	 */
	template<typename TNumeric, int32 NumberOfComponents, typename TOnValueGet, typename TOnValueSet>
		requires
			std::is_arithmetic_v<TNumeric> &&
			std::invocable<TOnValueSet, UE::Math::TVector<TNumeric>> &&
			std::same_as<std::invoke_result_t<TOnValueGet>, UE::Math::TVector<TNumeric>>
	FRuntimePropertyBuilder& AddNumericVector(FStringView PropertyName, TOnValueGet&& OnValueGet, TOnValueSet&& OnValueSet)
	{
		using FVectorType = UE::Math::TVector<TNumeric>;
		using SVectorWidget = SNumericVectorInputBox<TNumeric, FVectorType, NumberOfComponents>;

		// Macro to avoid code duplication for X, Y, Z component handlers
		// Creates a lambda that reconstructs the vector with the modified component
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

	/**
	 * Adds a rotator property editor (FRotator).
	 * 
	 * Creates a three-field input for Pitch, Yaw, and Roll with real-time updates.
	 * 
	 * @tparam TNumeric - Component type (usually float or double)
	 * @tparam TOnValueGet - Callable returning TRotator<TNumeric>
	 * @tparam TOnValueSet - Callable accepting TRotator<TNumeric>
	 * 
	 * @param PropertyName - Display name
	 * @param OnValueGet - Getter for current rotation
	 * @param OnValueSet - Setter called when any rotation component changes
	 * 
	 * @return Reference to this builder for chaining
	 * 
	 * Example:
	 * @code
	 * Builder.AddNumericRotator<double>("Rotation",
	 *     [this]() { return GetActorRotation(); },
	 *     [this](FRotator Rot) { SetActorRotation(Rot); });
	 * @endcode
	 */
	template<typename TNumeric, typename TOnValueGet, typename TOnValueSet>
		requires
			std::is_arithmetic_v<TNumeric> &&
			std::invocable<TOnValueSet, UE::Math::TRotator<TNumeric>> &&
			std::same_as<std::invoke_result_t<TOnValueGet>, UE::Math::TRotator<TNumeric>>
	FRuntimePropertyBuilder& AddNumericRotator(FStringView PropertyName, TOnValueGet&& OnValueGet, TOnValueSet&& OnValueSet)
	{
		// Macro for component change handlers (Pitch, Yaw, Roll)
		#define ON_VALUE_CHANGED_COMP(Comp, ...)						 \
		On##Comp##Changed_Lambda([OnValueGet, OnValueSet](TNumeric Comp) \
		{																 \
			FRotatorType OldRotator{ OnValueGet() };					 \
			OnValueSet({ __VA_ARGS__ });								 \
		})																 \

		// Macro for component getters (displays current values)
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

	/**
	 * Adds a boolean property editor (checkbox).
	 * 
	 * @tparam TOnValueGet - Callable returning bool
	 * @tparam TOnValueSet - Callable accepting bool
	 * 
	 * @param PropertyName - Display name
	 * @param OnValueGet - Getter for current boolean state
	 * @param OnValueSet - Setter called when checkbox is toggled
	 * 
	 * @return Reference to this builder for chaining
	 * 
	 * Example:
	 * @code
	 * Builder.AddBool("Is Active",
	 *     [this]() { return bIsActive; },
	 *     [this](bool Val) { bIsActive = Val; });
	 * @endcode
	 */
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

	/**
	 * Adds an action button that executes a callback when clicked.
	 * 
	 * Unlike properties, buttons don't have getters/setters - they simply execute code.
	 * Useful for triggering actions like "Reset", "Destroy", "TakeDamage", etc.
	 * 
	 * @tparam TOnClicked - Callable with no parameters (lambda, function pointer, etc.)
	 * 
	 * @param PropertyName - Button label text
	 * @param OnClicked - Callback executed when button is clicked
	 * 
	 * @return Reference to this builder for chaining
	 * 
	 * Example:
	 * @code
	 * Builder.AddButton("Reset Position", [this]() {
	 *     SetActorLocation(FVector::ZeroVector);
	 * });
	 * @endcode
	 */
	template<typename TOnClicked>
		requires std::invocable<TOnClicked>
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

	/**
	 * Adds a visual category header to organize properties.
	 * 
	 * Categories are displayed as centered, bold text to separate groups of properties.
	 * 
	 * @param CategoryName - Display text for the category
	 * 
	 * @return Reference to this builder for chaining
	 * 
	 * Example:
	 * @code
	 * Builder
	 *     .AddCategory("Movement")
	 *     .AddNumeric<float>("Speed", ...)
	 *     .AddCategory("Combat")
	 *     .AddNumeric<int32>("Health", ...);
	 * @endcode
	 */
	FRuntimePropertyBuilder& AddCategory(FStringView CategoryName);

	/**
	 * Adds a horizontal separator line to visually divide sections.
	 * 
	 * @param ColorAndOpacity - Color of the separator line (default uses Slate theme)
	 * @param Thickness - Line thickness in pixels
	 * 
	 * @return Reference to this builder for chaining
	 */
	FRuntimePropertyBuilder& AddSeparator(const FSlateColor& ColorAndOpacity, float Thickness);

	/**
	 * Adds a full-width custom widget (no label).
	 * 
	 * Use this for complex UI elements that need the entire row width.
	 * 
	 * @param PropertyWidget - Any Slate widget to display
	 * 
	 * @return Reference to this builder for chaining
	 */
	FRuntimePropertyBuilder& AddRowProperty(const TSharedRef<SWidget>& PropertyWidget);
	
	/**
	 * Adds a labeled property row (label on left, widget on right).
	 * 
	 * This is the base method used internally by AddNumeric, AddBool, etc.
	 * 
	 * @param PropertyName - Label text shown on the left
	 * @param PropertyWidget - Widget shown on the right (input control)
	 * 
	 * @return Reference to this builder for chaining
	 */
	FRuntimePropertyBuilder& AddRowProperty(FStringView PropertyName, const TSharedRef<SWidget>& PropertyWidget);

private:
	/** The Slate container widget that holds all added properties */
	TSharedPtr<SScrollBox> PropertiesContainer;
};