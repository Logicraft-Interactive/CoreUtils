// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Simple helper to retrieve wanted font style.
 */
class LOGICRAFTCOREUTILS_API SlateFontStyleHelper
{
public:
	/**
	 * This function retrieves the desired font style by using the FontPath.
	 * Beware: this function does not contain any security if the requested font is not valid.
	 * @param FontPath The used path to load the font.
	 * @param FontSize The font size.
	 * @return The needed font information.
	 */
	static FSlateFontInfo GetFontStyle(FStringView FontPath, float FontSize);

	/**
	 * This function retrieves the desired font style by using the FontPath.
	 * Instead of GetFontStyle, which has no security, this one contains security.
	 * If the requested font is not valid, a fallback will be made to Roboto-Regular.
	 * @param FontPath The path used to load the font.
	 * @param FontSize The font size.
	 * @return The needed font information.
	 */
	static FSlateFontInfo GetFontStyleSafe(const FString& FontPath, float FontSize);


	/**
	 * This function retrieves the desired Slate font style by using the FontPath.
	 * List of the available fonts:
	 * - Roboto-Regular.ttf
	 * - Roboto-Bold.ttf
	 * - Roboto-Italic.ttf
	 * - Roboto-BoldItalic.ttf
	 * - Roboto-Light.ttf
	 * - Roboto-Medium.ttf
	 * Beware: this function does not contain any security if the requested font is not valid.
	 * @param FontName The name used to load the font.
	 * @param FontSize The font size.
	 * @return The needed information of the font.
	 */
	static FSlateFontInfo GetSlateFontStyle(FStringView FontName, float FontSize);
	/**
	 * This function retrieves the desired Slate font style by using the FontPath.
	 * List of the available fonts:
	 * - Roboto-Regular.ttf
	 * - Roboto-Bold.ttf
	 * - Roboto-Italic.ttf
	 * - Roboto-BoldItalic.ttf
	 * - Roboto-Light.ttf
	 * - Roboto-Medium.ttf
	 * Instead of GetSlateFontStyle, which has no security, this one contains security.
	 * If the requested font is not valid, a fallback will be made to Roboto-Regular.
	 * @param FontName The name used to load the font.
	 * @param FontSize The font size.
	 * @return The needed information of the font.
	 */
	static FSlateFontInfo GetSlateFontStyleSafe(FStringView FontName, float FontSize);
};
