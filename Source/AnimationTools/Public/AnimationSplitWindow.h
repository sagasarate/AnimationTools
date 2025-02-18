#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWindow.h"
#include "Misc/Optional.h" 

struct FAnimSplitEntry : public FTableRowBase
{
	FString SequenceName;
	int32 StartFrame;
	int32 EndFrame;

	TSharedPtr<SButton> DeleteButton;
};

class SAnimationSplitWindow : public SWindow
{
public:
	SLATE_BEGIN_ARGS(SAnimationSplitWindow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	static void OpenWindow(UAnimSequence* Anim);

private:
	TSharedPtr<class SVerticalBox> EntriesBox;
	TArray<TSharedPtr<FAnimSplitEntry>> SplitEntries;
	TWeakObjectPtr<class UAnimSequence> SelectedAnimSequence;
	FString SaveDir;
	TSharedPtr<SEditableTextBox> SaveDirTextBox;

	FVector2D CalculateButtonSize(FName IconName, FName ButtonStyleName, FMargin ExtraPadding);
	void AddEntry(const FString& Name = TEXT(""), int32 StartFrame = 0, int32 EndFrame = 0, bool RefreshShow = true);
	void RebuildEntriesBox();
	FReply OnSplitClicked();
	FReply OnSaveDirClicked();
	void OnSaveDirSelected(const FString& SelectDir);
	bool ValidateEntries() const;

	void ImportFromCSV();
	void ExportToCSV();
	bool StringToEntries(FString& Str);
};
