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
	void ExecuteSplitAnimation(const TArray<FAssetData>& SelectedAssets);
};

DECLARE_LOG_CATEGORY_EXTERN(AnimationTools, Log, All)