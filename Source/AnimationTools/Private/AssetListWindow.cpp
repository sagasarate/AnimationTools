// SAssetListWindow.cpp
#include "AssetListWindow.h"
#include "AnimationTools.h"

#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Styling/AppStyle.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Framework/Application/SlateApplication.h"

#define LOCTEXT_NAMESPACE "AnimationTools"

void SAssetListWindow::OpenWindow(const FText& Title, const TArray<TSharedPtr<FAssetData>>& InAssets)
{
	TSharedRef<SAssetListWindow> Window = SNew(SAssetListWindow);
	Window->SetTitle(Title);
	for (const TSharedPtr<FAssetData>& Asset : InAssets)
	{
		Window->m_AssetList.Add(Asset);
	}
	if (Window->m_ListViewWidget.IsValid())
	{
		Window->m_ListViewWidget->RequestListRefresh();
	}
	FSlateApplication::Get().AddWindow(Window);
}

void SAssetListWindow::Construct(const FArguments& InArgs)
{
	SWindow::Construct(SWindow::FArguments()
		.Title(LOCTEXT("AssetListTitle", "Asset List"))
		.ClientSize(FVector2D(300, 400))
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		[
			SNew(SBorder)
				.Padding(FMargin(10))
				[
					SNew(SVerticalBox)
						+ SVerticalBox::Slot().FillHeight(1.0).Padding(5)
						[
							SAssignNew(m_ListViewWidget, SListView<TSharedPtr<FAssetData>>)
								//.ItemHeight(24)
								.ListItemsSource(&m_AssetList)
								.SelectionMode(ESelectionMode::None)
								.OnGenerateRow(this, &SAssetListWindow::OnGenerateRow)
						]
				]
		]

	);
}

TSharedRef<ITableRow> SAssetListWindow::OnGenerateRow(TSharedPtr<FAssetData> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FAssetData>>, OwnerTable)
		[
			SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
						.Text(FText::FromName(InItem->AssetName))
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(FMargin(4, 0, 0, 0))
				[
					SNew(SButton)
						.ButtonStyle(FAppStyle::Get(), "SimpleButton")
						.ContentPadding(2)
						.ToolTipText(FText::FromString(TEXT("Locate in Content Browser")))
						.OnClicked_Lambda([InItem]()
							{
								FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
								ContentBrowserModule.Get().SyncBrowserToAssets({ *InItem });
								return FReply::Handled();
							})
						[
							SNew(SImage)
								.Image(FAppStyle::Get().GetBrush("ContentBrowser.ShowInExplorer"))
						]
				]
		];
}

#undef LOCTEXT_NAMESPACE