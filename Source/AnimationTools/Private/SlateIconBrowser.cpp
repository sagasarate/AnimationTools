// SlateIconBrowser.cpp
#include "SlateIconBrowser.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Images/SImage.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformApplicationMisc.h"


#define LOCTEXT_NAMESPACE "AnimationTools"


void SSlateIconBrowser::Construct(const FArguments& InArgs)
{
	SWindow::Construct(SWindow::FArguments()
		.Title(LOCTEXT("WindowTitle", "Slate Icon Browser"))
		.ClientSize(FVector2D(800, 600))
		[
			SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(5)
				[
					SAssignNew(SearchBox, SSearchBox)
						.OnTextChanged_Lambda([this](const FText& Text) {
						RefreshIconList(Text.ToString());
							})
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					SAssignNew(ListView, SListView<TSharedPtr<FName>>)
						.ListItemsSource(&FilteredIcons)
						.OnGenerateRow(this, &SSlateIconBrowser::GenerateIconRow)
						.SelectionMode(ESelectionMode::None)
				]
		]);

	// 获取所有图标资源

	auto StyleKeys = FAppStyle::Get().GetStyleKeys();

	TArray<const FSlateBrush*> AllResources;
	FAppStyle::Get().GetResources(AllResources);

	// 过滤出图标（根据命名规则）
	for (auto key : StyleKeys)
	{
		const FSlateBrush* brush = FAppStyle::Get().GetBrush(key);
		if (brush)
		{
			IconNames.Add(key);
		}
	}

	RefreshIconList();
}

void SSlateIconBrowser::RefreshIconList(const FString& FilterText)
{
	FilteredIcons.Empty();

	for (const FName& IconName : IconNames)
	{
		if (FilterText.IsEmpty() ||
			IconName.ToString().Contains(FilterText))
		{
			FilteredIcons.Add(MakeShared<FName>(IconName));
		}
	}

	ListView->RequestListRefresh();
}

TSharedRef<ITableRow> SSlateIconBrowser::GenerateIconRow(
	TSharedPtr<FName> Item,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FName>>, OwnerTable)
		.Padding(2)
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5)
				[
					SNew(SBox)
						.WidthOverride(16)
						.HeightOverride(16)
						[
							SNew(SImage)
								.Image(FAppStyle::Get().GetBrush(*Item))
						]
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
						.Text(FText::FromName(*Item))
						.HighlightText_Lambda([this]() {
						return SearchBox->GetText();
							})
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5, 0)
				[
					SNew(SButton)
						.Text(LOCTEXT("CopyButton", "Copy"))
						.OnClicked_Lambda([Item]() {
						FPlatformApplicationMisc::ClipboardCopy(*Item->ToString());
						return FReply::Handled();
							})
				]
		];
}

#undef LOCTEXT_NAMESPACE