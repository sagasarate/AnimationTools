#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWindow.h"
#include "AssetRegistry/AssetData.h"
#include "Widgets/Layout/SWrapBox.h"

struct FAssetInfo
{
    TSharedPtr<FAssetData> Asset;
    TSharedPtr<FAssetThumbnail> Thumbnail;
    TArray<TSharedPtr<FAssetData>> Clips;
};

class SMeshBrowser : public SWindow
{
public:
    SLATE_BEGIN_ARGS(SMeshBrowser) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    static void OpenWindow(const TArray<FAssetData>& MeshAssets);
private:
    TArray<FAssetInfo> m_MeshAssetItems;
    TSharedPtr<FAssetThumbnailPool> m_ThumbnailPool;
    TSharedPtr<SWrapBox> m_Contents;
    TArray<FAssetInfo*> m_ThumbnailRefreshQueue;

    void SetAssets(const TArray<FAssetData>& MeshAssets);
    void Refresh();
    TSharedPtr<SWidget> OnGenerateContextMenu();
    void ShowAssetInfo(TSharedPtr<FAssetData> SelectedAsset);
    void RefreshThumbnail(FAssetInfo* pAssetInfo);
    FReply RefreshAllThumbnail();
    void DoThumbnailRefresh();
    void OnThumbnailRendered(const FAssetData& AssetData);
};
