// Copyright Epic Games, Inc. All Rights Reserved.

#include "NeuralAnimationToolkitStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FNeuralAnimationToolkitStyle::StyleInstance = nullptr;

void FNeuralAnimationToolkitStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FNeuralAnimationToolkitStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FNeuralAnimationToolkitStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("NeuralAnimationToolkitStyle"));
	return StyleSetName;
}

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef< FSlateStyleSet > FNeuralAnimationToolkitStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("NeuralAnimationToolkitStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("NeuralAnimationToolkit")->GetBaseDir() / TEXT("Resources"));

	Style->Set("NeuralAnimationToolkit.OpenPluginWindow", new IMAGE_BRUSH_SVG(TEXT("PlaceholderButtonIcon"), Icon20x20));

	return Style;
}

void FNeuralAnimationToolkitStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FNeuralAnimationToolkitStyle::Get()
{
	return *StyleInstance;
}
