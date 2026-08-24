// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FAnimationToolsModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	TSharedRef<FExtender> OnExtendAssetContextMenu(const TArray<FAssetData>& SelectedAssets);
	TSharedRef<FExtender> OnExtendFolderContentMenu(const TArray<FString>& SelectedPaths);
	TSharedRef<FExtender> OnExtendContentMenu(const TArray<FString>& SelectedPaths);
	void ExecuteSplitAnimation(const TArray<FAssetData>& SelectedAssets);
	void OnShowMeshBrowser(TArray<FString> SelectedPaths);
protected:
	void OnImportFBXClicked();
	void OnImportUnityParticlesClicked();
	void OnBrowseSlateIconClicked();
	void OnModelToIconClicked();
	void OnMeshLODSettingClicked();
};

DECLARE_LOG_CATEGORY_EXTERN(AnimationTools, Log, All)