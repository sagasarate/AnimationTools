// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimationTools.h"
#include "ContentBrowserModule.h"
#include "Animation/AnimSequence.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "AnimationSplitWindow.h"


DEFINE_LOG_CATEGORY(AnimationTools);

#define LOCTEXT_NAMESPACE "AnimationTools"



void FAnimationToolsModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	TArray<FContentBrowserMenuExtender_SelectedAssets>& MenuExtenders = ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
	MenuExtenders.Add(FContentBrowserMenuExtender_SelectedAssets::CreateRaw(this, &FAnimationToolsModule::OnExtendAssetContextMenu));	
}

void FAnimationToolsModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

TSharedRef<FExtender> FAnimationToolsModule::OnExtendAssetContextMenu(const TArray<FAssetData>& SelectedAssets)
{
	TSharedRef<FExtender> Extender(new FExtender());
	bool bIsAnimSequenceSelected = SelectedAssets.Num() > 0 && SelectedAssets[0].GetClass() == UAnimSequence::StaticClass();

	if (bIsAnimSequenceSelected)
	{

		Extender->AddMenuExtension(
			"CommonAssetActions",
			EExtensionHook::After,
			nullptr,
			FMenuExtensionDelegate::CreateLambda([this, SelectedAssets](FMenuBuilder& MenuBuilder)
				{
					MenuBuilder.AddMenuEntry(
						LOCTEXT("split_anim", "Split Animation"),
						LOCTEXT("split_anim_desc", "Split the selected animation into segments."),
						FSlateIcon(FAppStyle::Get().GetStyleSetName(), "Icons.Crop"),
						FExecuteAction::CreateLambda([this, SelectedAssets]()
							{
								this->ExecuteSplitAnimation(SelectedAssets);
							})
					);
				})
		);
	}
	return Extender;
}

void FAnimationToolsModule::ExecuteSplitAnimation(const TArray<FAssetData>& SelectedAssets)
{
	UAnimSequence* AnimSequence = Cast<UAnimSequence>(SelectedAssets[0].GetAsset());
	if (!AnimSequence)
		return;


	SAnimationSplitWindow::OpenWindow(AnimSequence);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAnimationToolsModule, AnimationTools)