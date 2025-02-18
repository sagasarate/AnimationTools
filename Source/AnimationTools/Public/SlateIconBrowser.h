// SlateIconBrowser.h
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWindow.h"

class SSlateIconBrowser : public SWindow
{
public:
	SLATE_BEGIN_ARGS(SSlateIconBrowser) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TArray<FName> IconNames;
	TArray<TSharedPtr<FName> > FilteredIcons;
	TSharedPtr<SSearchBox> SearchBox;
	TSharedPtr<SListView<TSharedPtr<FName>>> ListView;

	void RefreshIconList(const FString& FilterText = "");
	TSharedRef<ITableRow> GenerateIconRow(TSharedPtr<FName> Item, const TSharedRef<STableViewBase>& OwnerTable);
};
