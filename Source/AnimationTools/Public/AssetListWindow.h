#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWindow.h"
#include "AssetRegistry/AssetData.h"

class SAssetListWindow : public SWindow
{
public:
	
	static void OpenWindow(const FText& Title, const TArray<TSharedPtr<FAssetData>>& InAssets);
	void Construct(const FArguments& InArgs);

private:
	
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FAssetData> InItem, const TSharedRef<STableViewBase>& OwnerTable);

private:
	TSharedPtr<SListView<TSharedPtr<FAssetData>>> m_ListViewWidget;
	TArray<TSharedPtr<FAssetData>> m_AssetList;
};
