#include "MeshBrowser.h"
#include "AnimationTools.h"

#include "Widgets/Views/SListView.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SScrollBox.h"

#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "AssetThumbnail.h"
#include "ThumbnailRendering/ThumbnailRenderer.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "PackageTools.h"
#include "ObjectTools.h"
#include "UObject/SavePackage.h"

#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"

#include "AssetListWindow.h"
#include "SlateIconBrowser.h"

#define LOCTEXT_NAMESPACE "AnimationTools"

void SMeshBrowser::Construct(const FArguments& InArgs)
{
	SWindow::Construct(SWindow::FArguments()
		.Title(LOCTEXT("MeshBrowserTitle", "Mesh Browser"))
		.ClientSize(FVector2D(800, 600))
		.SupportsMaximize(true)
		.SupportsMinimize(true)
		[
			SNew(SBorder)
				.Padding(FMargin(10))
				[
					SNew(SVerticalBox)
						+ SVerticalBox::Slot().FillHeight(1.0).Padding(5)
						[
							SNew(SScrollBox)
								+ SScrollBox::Slot()
								[
									SAssignNew(m_Contents, SWrapBox)
										.UseAllottedSize(true)
										//.HAlign(HAlign_Fill)
								]
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(5)
						[
							SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().Padding(2)
								[
									SNew(SButton)
										.Text(LOCTEXT("RefreshAllThumbnail", "Refresh All Thumbnail"))
										.OnClicked(this, &SMeshBrowser::RefreshAllThumbnail)
								]
						]
				]
		]
	);
}

void SMeshBrowser::OpenWindow(const TArray<FAssetData>& MeshAssets)
{
	TSharedRef<SMeshBrowser> Window = SNew(SMeshBrowser);

	Window->m_ThumbnailPool = MakeShared<FAssetThumbnailPool>(MeshAssets.Num());
	Window->m_ThumbnailPool->OnThumbnailRendered().AddSP(Window, &SMeshBrowser::OnThumbnailRendered);
	Window->SetAssets(MeshAssets);

	FSlateApplication::Get().AddWindow(Window);
}

void SMeshBrowser::SetAssets(const TArray<FAssetData>& MeshAssets)
{
	m_MeshAssetItems.Empty();
	for (const FAssetData& AssetData : MeshAssets)
	{
		FAssetInfo& AssetInfo = m_MeshAssetItems.AddZeroed_GetRef();
		AssetInfo.Asset = MakeShared<FAssetData>(AssetData);		
	}
	m_MeshAssetItems.Sort([](const FAssetInfo& Info1, const FAssetInfo& Info2)->bool {return Info1.Asset->AssetName.Compare(Info2.Asset->AssetName) < 0; });

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	for (FAssetInfo& AssetInfo : m_MeshAssetItems)
	{
		AssetInfo.Thumbnail = MakeShared<FAssetThumbnail>(*AssetInfo.Asset, 128, 128, m_ThumbnailPool);

		TArray<FAssetData> AnimAssets;		

		FARFilter Filter;
		Filter.bRecursivePaths = true;
		FString AnimsPath = FPaths::Combine(AssetInfo.Asset->PackagePath.ToString(), TEXT("Anims"));
		Filter.PackagePaths.Add(FName(*AnimsPath));
		Filter.ClassPaths.Add(UAnimSequence::StaticClass()->GetClassPathName());

		AssetRegistryModule.Get().GetAssets(Filter, AnimAssets);
		AssetInfo.Clips.Reserve(AnimAssets.Num());
		for (FAssetData& AssetData : AnimAssets)
		{
			AssetInfo.Clips.Add(MakeShared<FAssetData>(AssetData));
		}
	}
	Refresh();
}

void SMeshBrowser::Refresh()
{
	if (!m_Contents.IsValid())
		return;

	FAssetThumbnailConfig ThumbnailConfig;
	ThumbnailConfig.bAllowRealTimeOnHovered = false;

	for (FAssetInfo& AssetInfo : m_MeshAssetItems)
	{
		m_Contents->AddSlot().Padding(5)
			[
				SNew(SBorder)
					.Padding(5)
					.OnMouseButtonDown_Lambda([this, &AssetInfo](const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
						{
							if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
							{
								FMenuBuilder MenuBuilder(true, nullptr);
								MenuBuilder.AddMenuEntry(
									LOCTEXT("OpenAsset", "Open Asset"),
									LOCTEXT("OpenAsset", "Open Asset"),
									FSlateIcon(),
									FUIAction(FExecuteAction::CreateLambda([&AssetInfo]()
										{
											if (UObject* Asset = AssetInfo.Asset->GetAsset())
											{
												if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
												{
													AssetEditorSubsystem->OpenEditorForAsset(Asset);
												}
											}
										}))
								);
								MenuBuilder.AddMenuEntry(
									LOCTEXT("DetailInfo", "Show Detail Info"),
									LOCTEXT("DetailInfo", "Show Detail Info"),
									FSlateIcon(),
									FUIAction(FExecuteAction::CreateLambda([&AssetInfo]()
										{
											SAssetListWindow::OpenWindow(LOCTEXT("TitleAnimList", "Anim List"), AssetInfo.Clips);
										}))
								);
								MenuBuilder.AddMenuEntry(
									LOCTEXT("RefreshThumbnail", "Refresh Thumbnail"),
									LOCTEXT("RefreshThumbnail", "Refresh Thumbnail"),
									FSlateIcon(),
									FUIAction(FExecuteAction::CreateLambda([this, &AssetInfo]()
										{
											RefreshThumbnail(&AssetInfo);
										}))
								);
								MenuBuilder.AddMenuEntry(
									LOCTEXT("LocateAsset", "Locate In Content Browser"),
									LOCTEXT("LocateAsset", "Locate In Content Browser"),
									FSlateIcon(),
									FUIAction(FExecuteAction::CreateLambda([this, &AssetInfo]()
										{
											FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
											ContentBrowserModule.Get().SyncBrowserToAssets({ *AssetInfo.Asset });
										}))
								);

								FSlateApplication::Get().PushMenu(
									AsShared(),
									FWidgetPath(),
									MenuBuilder.MakeWidget(),
									MouseEvent.GetScreenSpacePosition(),
									FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu)
								);

								return FReply::Handled();
							}
							return FReply::Unhandled();
						})
					[
						SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SBox)
									[
										AssetInfo.Thumbnail->MakeThumbnailWidget(ThumbnailConfig)
									]
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SBox)
									.WidthOverride(128)
									[
										SNew(STextBlock)
											.Text(FText::Format(FText::FromString("{0}({1})"), FText::FromName(AssetInfo.Asset->AssetName), AssetInfo.Clips.Num()))
											.AutoWrapText(true)
											.Justification(ETextJustify::Center)
											.WrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping)
									]
							]
					]
			];
	}
}



TSharedPtr<SWidget> SMeshBrowser::OnGenerateContextMenu()
{
	FMenuBuilder MenuBuilder(true, nullptr);
	MenuBuilder.AddMenuEntry(
		FText::FromString("查看详情"),
		FText::FromString("显示资源路径"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &SMeshBrowser::ShowAssetInfo, m_MeshAssetItems[0].Asset))
	);

	return MenuBuilder.MakeWidget();
}

void SMeshBrowser::ShowAssetInfo(TSharedPtr<FAssetData> SelectedAsset)
{

}

void SMeshBrowser::RefreshThumbnail(FAssetInfo* pAssetInfo)
{
	m_ThumbnailRefreshQueue.Add(pAssetInfo);
	if (m_ThumbnailRefreshQueue.Num() == 1)
		DoThumbnailRefresh();
}

FReply SMeshBrowser::RefreshAllThumbnail()
{
	m_ThumbnailRefreshQueue.Reserve(m_MeshAssetItems.Num());
	for (FAssetInfo& Info : m_MeshAssetItems)
		m_ThumbnailRefreshQueue.Add(&Info);
	DoThumbnailRefresh();
	return FReply::Handled();
}

void SMeshBrowser::DoThumbnailRefresh()
{
	if (m_ThumbnailRefreshQueue.Num())
	{
		FAssetInfo* pAssetInfo = m_ThumbnailRefreshQueue[m_ThumbnailRefreshQueue.Num() - 1];
		UObject* Asset = pAssetInfo->Asset->GetAsset();
		pAssetInfo->Thumbnail->RefreshThumbnail();
	}
}

void SMeshBrowser::OnThumbnailRendered(const FAssetData& AssetData)
{
	UE_LOG(AnimationTools, Warning, TEXT("Thumbnail Rendered:%s"), *AssetData.GetObjectPathString());
	if (m_ThumbnailRefreshQueue.Num() &&
		m_ThumbnailRefreshQueue[m_ThumbnailRefreshQueue.Num() - 1]->Asset->GetObjectPathString() == AssetData.GetObjectPathString())
	{
		m_ThumbnailRefreshQueue.Pop();
		UObject* Asset = AssetData.GetAsset();
		Asset->MarkPackageDirty();
		DoThumbnailRefresh();
	}
}

#undef LOCTEXT_NAMESPACE