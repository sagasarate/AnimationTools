#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "ImportProgressWin.h"

#include "FBXBatchImportWin.generated.h"

class SFBXBatchImportWin;

USTRUCT()
struct FClipInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FString Name;

	UPROPERTY()
	int32 StartFrame = 0;

	UPROPERTY()
	int32 EndFrame = 0;
};

USTRUCT()
struct FAniInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FString ModelPath;

	UPROPERTY()
	TArray<FClipInfo> Clips;
};

/** 结构体：FBX 文件项 */
struct FFBXFileItem
{
	FString FilePath; // 源目录相对路径
	bool bShouldImport = true;
	TArray<FClipInfo> Clips;
};

/** FBX 批量导入窗口 */
class SFBXBatchImportWin : public SWindow
{
public:
	SLATE_BEGIN_ARGS(SFBXBatchImportWin) {}
	SLATE_END_ARGS()

	/** 构造函数 */
	void Construct(const FArguments &InArgs);
	static void OpenWindow();

private:
	/** 绑定到按钮的回调函数 */
	FReply OnSelectSourceDir();
	FReply OnSelectTargetDir();
	void OnTargetDirSelected(const FString &SelectDir);
	FReply OnSelectJsonFile();
	FReply OnSearchFBXFiles();
	FReply OnMatchAnimationData();
	FReply OnExecuteImport();
	void OnSelectAllChanged(ECheckBoxState NewState);
	void CheckClips(TArray<FClipInfo> &Clips);
	bool DoSplit(UAnimSequence *AnimSequence, const TArray<FClipInfo> Clips, const FString &SaveDir, TArray<UObject *> &Assets, float Progress);
	void DeleteAssetPackage(const FString &PackagePath);
	/** 获取 FBX 文件项的文本 */
	TSharedRef<ITableRow> OnGenerateRowForListView(TSharedPtr<FFBXFileItem> Item, const TSharedRef<STableViewBase> &OwnerTable);

	void OnModelTick(float DeltaTime);

	/** 源目录、目标目录、JSON 文件路径 */
	FString m_SourceDir;
	FString m_TargetDir;
	FString m_JsonFilePath;

	bool m_bAllSelected = true;

	TSharedPtr<SEditableTextBox> m_SourceDirTextBox;
	TSharedPtr<SEditableTextBox> m_TargetDirTextBox;
	TSharedPtr<SEditableTextBox> m_JsonFilePathTextBox;
	TSharedPtr<SImportProgressWin> m_ProgressWindow;

	/** FBX 文件列表 */
	TArray<TSharedPtr<FFBXFileItem>> m_FBXFileList;
	TSharedPtr<SListView<TSharedPtr<FFBXFileItem>>> m_ListViewWidget;
};
