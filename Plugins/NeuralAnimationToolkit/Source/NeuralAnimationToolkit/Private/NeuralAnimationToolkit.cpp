// Copyright Epic Games, Inc. All Rights Reserved.

#include "NeuralAnimationToolkit.h"
#include "NeuralAnimationToolkitStyle.h"
#include "NeuralAnimationToolkitCommands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"

static const FName NeuralAnimationToolkitTabName("NeuralAnimationToolkit");

#define LOCTEXT_NAMESPACE "FNeuralAnimationToolkitModule"

void FNeuralAnimationToolkitModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FNeuralAnimationToolkitStyle::Initialize();
	FNeuralAnimationToolkitStyle::ReloadTextures();

	FNeuralAnimationToolkitCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FNeuralAnimationToolkitCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FNeuralAnimationToolkitModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FNeuralAnimationToolkitModule::RegisterMenus));
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(NeuralAnimationToolkitTabName, FOnSpawnTab::CreateRaw(this, &FNeuralAnimationToolkitModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FNeuralAnimationToolkitTabTitle", "NeuralAnimationToolkit"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FNeuralAnimationToolkitModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FNeuralAnimationToolkitStyle::Shutdown();

	FNeuralAnimationToolkitCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(NeuralAnimationToolkitTabName);
}

TSharedRef<SDockTab> FNeuralAnimationToolkitModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	FText WidgetText = FText::Format(
		LOCTEXT("WindowWidgetText", "Add code to {0} in {1} to override this window's contents"),
		FText::FromString(TEXT("FNeuralAnimationToolkitModule::OnSpawnPluginTab")),
		FText::FromString(TEXT("NeuralAnimationToolkit.cpp"))
		);

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			// Put your tab content here!
			SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(WidgetText)
			]
		];
}

void FNeuralAnimationToolkitModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(NeuralAnimationToolkitTabName);
}

void FNeuralAnimationToolkitModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FNeuralAnimationToolkitCommands::Get().OpenPluginWindow, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FNeuralAnimationToolkitCommands::Get().OpenPluginWindow));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FNeuralAnimationToolkitModule, NeuralAnimationToolkit)