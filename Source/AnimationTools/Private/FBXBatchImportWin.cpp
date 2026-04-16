#include "FBXBatchImportWin.h"
#include "AnimationTools.h"

#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Views/SListView.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"

#include "AssetToolsModule.h"
#include "EditorAssetLibrary.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "AssetImportTask.h"

#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"

#include "JsonObjectConverter.h"

#include "Factories/AnimSequenceFactory.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "PackageTools.h"
#include "ObjectTools.h"

#include "UnityParticleSystem.h"
#include "UObject/SavePackage.h"


#define LOCTEXT_NAMESPACE "AnimationTools"

/** Slate UI 构造 */
void SFBXBatchImportWin::Construct(const FArguments& InArgs)
{
	m_SourceDir = TEXT("E:/UnityPrj/loki_taitan/Assets/Models/Monster");
	m_TargetDir = TEXT("/Game/Models/Chars");
	m_JsonFilePath = TEXT("E:/UnityPrj/Models.json");
	SWindow::Construct(SWindow::FArguments()
		.Title(LOCTEXT("BatchFBXImportTitle", "Batch FBX Import"))
		.ClientSize(FVector2D(800, 500))
		.SizingRule(ESizingRule::UserSized)
		.AutoCenter(EAutoCenter::PreferredWorkArea)
		.SupportsMinimize(false)
		.SupportsMaximize(false)
		.HasCloseButton(true)
		.CreateTitleBar(true)
		[
			SNew(SBorder)
				.Padding(FMargin(10))
				[
					SNew(SVerticalBox)

						// 源目录选择
						+ SVerticalBox::Slot().AutoHeight().Padding(5)
						[
							SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth()
								[
									SNew(STextBlock)
										.Text(LOCTEXT("SourceDir", "Source Directory:"))
								]
								+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2)
								[
									SAssignNew(m_SourceDirTextBox, SEditableTextBox)
										.Text(FText::FromString(m_SourceDir))
										.OnTextChanged_Lambda([this](const FText& NewText) { m_SourceDir = NewText.ToString(); })
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(2)
								[
									SNew(SButton)
										.Text(FText::FromString("..."))
										.OnClicked(this, &SFBXBatchImportWin::OnSelectSourceDir)
								]
						]

						// 目标目录选择
						+ SVerticalBox::Slot().AutoHeight().Padding(5)
						[
							SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth()
								[
									SNew(STextBlock)
										.Text(LOCTEXT("TargetDir", "Target Directory:"))
								]
								+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2)
								[
									SAssignNew(m_TargetDirTextBox, SEditableTextBox)
										.Text(FText::FromString(m_TargetDir))
										.OnTextChanged_Lambda([this](const FText& NewText) { m_TargetDir = NewText.ToString(); })
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(2)
								[
									SNew(SButton)
										.Text(FText::FromString("..."))
										.OnClicked(this, &SFBXBatchImportWin::OnSelectTargetDir)
								]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(5)
						[
							SNew(SCheckBox)
								.OnCheckStateChanged(this, &SFBXBatchImportWin::OnSelectAllChanged)
								.IsChecked_Lambda([this]() { return m_bAllSelected ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
								[
									SNew(STextBlock)
										.Text(LOCTEXT("SelectAll", "Select All"))
								]
						]
						// FBX 文件列表
						+ SVerticalBox::Slot().FillHeight(1.0).Padding(5)
						[
							SAssignNew(m_ListViewWidget, SListView<TSharedPtr<FFBXFileItem>>)
								.ListItemsSource(&m_FBXFileList)
								.OnGenerateRow(this, &SFBXBatchImportWin::OnGenerateRowForListView)
								.HeaderRow(
									SNew(SHeaderRow)
									//+ SHeaderRow::Column("Import")
									//.DefaultLabel(LOCTEXT("Column_Import", "Import"))
									//.FixedWidth(50.0f)

									+SHeaderRow::Column("FilePath")
									.DefaultLabel(LOCTEXT("Column_FilePath", "FBX File Path"))
									.FillWidth(1.0f)

									+ SHeaderRow::Column("AnimationSlices")
									.DefaultLabel(LOCTEXT("Column_AnimationSlices", "Ani Clips"))
									.FixedWidth(60)
									.HAlignHeader(HAlign_Right)
								)
						]

						// JSON 文件选择
						+ SVerticalBox::Slot().AutoHeight().Padding(5)
						[
							SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth()
								[
									SNew(STextBlock)
										.Text(LOCTEXT("JsonFile", "Animation Split Data JSON:"))
								]
								+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2)
								[
									SAssignNew(m_JsonFilePathTextBox, SEditableTextBox)
										.Text(FText::FromString(m_JsonFilePath))
										.OnTextChanged_Lambda([this](const FText& NewText) { m_JsonFilePath = NewText.ToString(); })
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(2)
								[
									SNew(SButton)
										.Text(FText::FromString("..."))
										.OnClicked(this, &SFBXBatchImportWin::OnSelectJsonFile)
								]
						]

						// 按钮区
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(5)
						[
							SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().Padding(2)
								[
									SNew(SButton)
										.Text(LOCTEXT("SearchFBX", "Search FBX"))
										.OnClicked(this, &SFBXBatchImportWin::OnSearchFBXFiles)
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(2)
								[
									SNew(SButton)
										.Text(LOCTEXT("MatchData", "Match Animation Split Data"))
										.OnClicked(this, &SFBXBatchImportWin::OnMatchAnimationData)
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(2)
								[
									SNew(SButton)
										.Text(LOCTEXT("ExecuteImport", "Execute Import"))
										.OnClicked(this, &SFBXBatchImportWin::OnExecuteImport)
								]
						]
				]
		]
	);
}

void SFBXBatchImportWin::OpenWindow()
{
	TSharedRef<SFBXBatchImportWin> Window = SNew(SFBXBatchImportWin);
	FSlateApplication::Get().AddWindow(Window);
}

/** 生成 ListView 行 */
TSharedRef<ITableRow> SFBXBatchImportWin::OnGenerateRowForListView(TSharedPtr<FFBXFileItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FFBXFileItem>>, OwnerTable)
		[
			SNew(SHorizontalBox)

				// 导入复选框
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SCheckBox)
						.IsChecked_Lambda([Item]() { return Item->bShouldImport ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([Item](ECheckBoxState NewState) { Item->bShouldImport = (NewState == ECheckBoxState::Checked); })
				]

				// 文件路径
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(STextBlock)
						.Text(FText::FromString(Item->FilePath))
				]

				// 动画切片数
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(STextBlock)
						.MinDesiredWidth(120)
						.Justification(ETextJustify::Right)
						.Text(FText::AsNumber(Item->Clips.Num()))
				]
		];
}

/** 目录选择（示例） */
FReply SFBXBatchImportWin::OnSelectSourceDir()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		FString SelectedFolder;
		const void* ParentWindowHandle = GetNativeWindow()->GetOSWindowHandle();
		if (DesktopPlatform->OpenDirectoryDialog(ParentWindowHandle, LOCTEXT("SelectSourceFolder", "Select Source Folder").ToString(), m_SourceDir, SelectedFolder))
		{
			m_SourceDir = SelectedFolder;
			FPaths::NormalizeFilename(m_SourceDir);
			m_SourceDirTextBox->SetText(FText::FromString(m_SourceDir));
		}
	}
	return FReply::Handled();
}
FReply SFBXBatchImportWin::OnSelectTargetDir()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	FPathPickerConfig FolderDialogConfig;
	FolderDialogConfig.DefaultPath = m_TargetDir;
	FolderDialogConfig.bAddDefaultPath = true;
	FolderDialogConfig.OnPathSelected.BindRaw(this, &SFBXBatchImportWin::OnTargetDirSelected);

	TSharedPtr<SWidget> PathPickerWidget = ContentBrowserModule.Get().CreatePathPicker(FolderDialogConfig);

	if (PathPickerWidget.IsValid())
	{
		// 在新窗口中显示路径选择器
		TSharedPtr<SWindow> ParentWindow = SharedThis(this);
		FSlateApplication::Get().AddModalWindow(SNew(SWindow)
			.ClientSize(FVector2D(400, 400))
			.Content()
			[
				PathPickerWidget.ToSharedRef()
			], ParentWindow);
	}
	return FReply::Handled();
}

void SFBXBatchImportWin::OnTargetDirSelected(const FString& SelectDir)
{
	m_TargetDir = SelectDir;
	m_TargetDirTextBox->SetText(FText::FromString(m_TargetDir));
}

FReply SFBXBatchImportWin::OnSelectJsonFile()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		TArray<FString> OutFiles;
		const void* ParentWindowHandle = GetNativeWindow()->GetOSWindowHandle();
		FString FileTypes = TEXT("JSON Files (*.json)|*.json");

		if (DesktopPlatform->OpenFileDialog(ParentWindowHandle, LOCTEXT("SelectJsonFile", "Select Animation Data JSON").ToString(), m_JsonFilePath, TEXT(""), FileTypes, EFileDialogFlags::None, OutFiles))
		{
			if (OutFiles.Num() > 0)
			{
				m_JsonFilePath = OutFiles[0];
				FPaths::NormalizeFilename(m_JsonFilePath);
				m_JsonFilePathTextBox->SetText(FText::FromString(m_JsonFilePath));
			}
		}
	}
	return FReply::Handled();
}
FReply SFBXBatchImportWin::OnSearchFBXFiles()
{
	FString SrcDir = m_SourceDir.TrimStartAndEnd();
	FPaths::NormalizeFilename(SrcDir);
	if (!SrcDir.EndsWith(TEXT("/")))
	{
		SrcDir.Append(TEXT("/"));
	}

	m_FBXFileList.Empty();

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	PlatformFile.IterateDirectoryRecursively(*SrcDir, [&](const TCHAR* FilePath, bool bIsDirectory)
		{
			if (!bIsDirectory)
			{
				FString FileStr = FilePath;
				FString Extension = FPaths::GetExtension(FileStr, false);
				if (Extension.Equals(TEXT("fbx"), ESearchCase::IgnoreCase))
				{
					TSharedPtr<FFBXFileItem> pItem = MakeShared<FFBXFileItem>();
					m_FBXFileList.Add(pItem);

					pItem->FilePath = FilePath;
					FPaths::MakePathRelativeTo(pItem->FilePath, *SrcDir);
					pItem->bShouldImport = m_bAllSelected;
					pItem->Clips.Empty();
				}
			}
			return true;
		});

	// 刷新列表 UI
	if (m_ListViewWidget.IsValid())
	{
		m_ListViewWidget->RequestListRefresh();
	}
	return FReply::Handled();
}
FReply SFBXBatchImportWin::OnMatchAnimationData()
{
	if (m_JsonFilePath.IsEmpty())
	{
		UE_LOG(AnimationTools, Error, TEXT("JSON file path must be provided."));
		return FReply::Handled();
	}
	FString JsonString;
	if (FFileHelper::LoadFileToString(JsonString, *m_JsonFilePath))
	{
		TArray<FAniInfo> AnimClipInfos;
		if (FJsonObjectConverter::JsonArrayStringToUStruct(JsonString, &AnimClipInfos))
		{
			for (FAniInfo& AniInfo : AnimClipInfos)
			{
				FPaths::NormalizeFilename(AniInfo.ModelPath);
#pragma warning(push)
#pragma warning(disable: 4834)
				m_FBXFileList.IndexOfByPredicate([this, &AniInfo](TSharedPtr<FFBXFileItem> pItem) -> bool
					{
						if (pItem->FilePath.Compare(AniInfo.ModelPath, ESearchCase::IgnoreCase) == 0)
						{
							pItem->Clips = AniInfo.Clips;
							CheckClips(pItem->Clips);
							pItem->bShouldImport = !UEditorAssetLibrary::DoesAssetExist(FPaths::Combine(m_TargetDir, FPaths::GetPath(pItem->FilePath), FPaths::GetBaseFilename(pItem->FilePath)));
							return true;
						}
						return false;
					});
#pragma warning(pop)
			}
			// 刷新列表 UI
			if (m_ListViewWidget.IsValid())
			{
				m_ListViewWidget->RebuildList();
			}
		}
		else
		{
			UE_LOG(AnimationTools, Error, TEXT("Failed to parse JSON."));
		}
	}
	else
	{
		UE_LOG(AnimationTools, Error, TEXT("Failed to load JSON file: %s"), *m_JsonFilePath);
	}
	return FReply::Handled();
}


FReply SFBXBatchImportWin::OnExecuteImport()
{
	// 创建进度窗口
	m_ProgressWindow = SNew(SImportProgressWin);


	// 获取资产工具模块
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

	TArray<TSharedPtr<FFBXFileItem>> FilesToImport = m_FBXFileList.FilterByPredicate(
		[](const TSharedPtr<FFBXFileItem>& Item) { return Item->bShouldImport; }
	);
	int32 CurrentIndex = 0;

	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
		[this, FilesToImport, CurrentIndex, &AssetToolsModule](float DeltaTime) mutable -> bool
		{
			TSharedPtr<FFBXFileItem> pItem = FilesToImport[CurrentIndex++];
			FString SrcPath = FPaths::Combine(m_SourceDir, pItem->FilePath);
			float Progress = float(CurrentIndex) / FilesToImport.Num();

			if (pItem->bShouldImport)
			{
				FString TargetPath = FPaths::GetPath(FPaths::Combine(m_TargetDir, pItem->FilePath));

				FPaths::NormalizeFilename(SrcPath);
				FPaths::NormalizeFilename(TargetPath);

				// 创建导入任务
				UAssetImportTask* ImportTask = NewObject<UAssetImportTask>();
				ImportTask->Filename = SrcPath;
				ImportTask->DestinationPath = TargetPath;
				ImportTask->bAutomated = true;
				ImportTask->bSave = true;
				ImportTask->bReplaceExisting = true;

				// 设置FBX导入参数
				UFbxImportUI* ImportUI = NewObject<UFbxImportUI>();
				ImportUI->bImportMesh = true;
				ImportUI->bImportAsSkeletal = true;
				ImportUI->bImportAnimations = true;
				ImportUI->bImportMaterials = true;
				ImportUI->bImportTextures = true;
				ImportUI->bCreatePhysicsAsset = true;
				ImportTask->Options = ImportUI;

				FString AnimPath = FString::Printf(TEXT("%s/%s_%s"), *TargetPath, *FPaths::GetBaseFilename(pItem->FilePath), TEXT("Anim"));
				FString SaveDir = FPaths::Combine(FPaths::GetPath(AnimPath), TEXT("Anims"));


				m_ProgressWindow->UpdateProgress(Progress, FText::Format(LOCTEXT("CurImportFile", "Import: {0}"), FText::FromString(SrcPath)).ToString());

				//执行导入
				AssetToolsModule.Get().ImportAssetTasks({ ImportTask });

				TArray<UObject*> Assets = ImportTask->GetObjects();

				m_ProgressWindow->UpdateProgress(Progress, FText::Format(LOCTEXT("CurSplitFile", "Split: {0}"), FText::FromString(AnimPath)).ToString());

				//执行动画切片
				if (pItem->Clips.Num() > 1)
				{

					UAnimSequence* AnimSequence = Cast<UAnimSequence>(StaticLoadObject(UAnimSequence::StaticClass(), nullptr, *AnimPath));
					if (AnimSequence)
					{
						DoSplit(AnimSequence, pItem->Clips, SaveDir, Assets, Progress);
					}
				}
				m_ProgressWindow->UpdateProgress(Progress, LOCTEXT("GenerateThumbnail", "Generate Thumbnail").ToString());
				for (UObject* Asset : Assets)
				{
					if (Asset)
					{
						FString PackageName = Asset->GetOutermost()->GetName();
						UPackage* Package = FindPackage(nullptr, *PackageName);
						if (Package)
						{
							FString PackageFilePath = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
							FSavePackageArgs SaveArgs;
							SaveArgs.TopLevelFlags = EObjectFlags::RF_Public | EObjectFlags::RF_Standalone;
							UPackage::SavePackage(Package, Asset, *PackageFilePath, SaveArgs);
						}
					}
				}

				Assets.Empty();
				CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
			}
			if (CurrentIndex >= FilesToImport.Num())
			{
				m_ProgressWindow->CloseWindow();
				return false;
			}
			return true;
		}
	));


	//FVector2D MainWinPos = FSlateApplication::Get().GetActiveTopLevelWindow()->GetPositionInScreen();
	//FVector2D MainWinSize = FSlateApplication::Get().GetActiveTopLevelWindow()->GetSizeInScreen();
	//FVector2D ProgressWinSize = ProgressWindow->GetDesiredSize();

	//// **计算偏上一点的位置**
	//FVector2D NewPos = MainWinPos + FVector2D((MainWinSize.X - ProgressWinSize.X) / 2, (MainWinSize.Y - ProgressWinSize.Y) / 4);

	//// **设置窗口位置**
	//ProgressWindow->MoveWindowTo(NewPos);
	FVector2D Pos = m_ProgressWindow->GetPositionInScreen();

	FSlateApplication::Get().GetOnModalLoopTickEvent().AddRaw(this, &SFBXBatchImportWin::OnModelTick);

	FSlateApplication::Get().AddModalWindow(m_ProgressWindow.ToSharedRef(), FSlateApplication::Get().GetActiveTopLevelWindow());

	FSlateApplication::Get().GetOnModalLoopTickEvent().RemoveAll(this);

	m_ProgressWindow.Reset();

	return FReply::Handled();
}

void SFBXBatchImportWin::OnModelTick(float DeltaTime)
{
	if (FSlateApplication::Get().GetActiveModalWindow() == m_ProgressWindow)
		FTSTicker::GetCoreTicker().Tick(DeltaTime);
}

void SFBXBatchImportWin::OnSelectAllChanged(ECheckBoxState NewState)
{
	m_bAllSelected = (NewState == ECheckBoxState::Checked);

	// 选择/取消选择所有项
	for (TSharedPtr<FFBXFileItem>& Item : m_FBXFileList)
	{
		Item->bShouldImport = m_bAllSelected;
	}

	// 刷新列表 UI
	if (m_ListViewWidget.IsValid())
	{
		m_ListViewWidget->RequestListRefresh();
	}
}
void SFBXBatchImportWin::CheckClips(TArray<FClipInfo>& Clips)
{
	//对Clips进行去重
	for (int32 i = Clips.Num() - 1; i >= 0; i--)
	{
		int32 j = 0;
		for (; j < i; j++)
		{
			if (Clips[i].Name == Clips[j].Name)
				break;
		}
		if (j < i)
		{
			if (Clips[i].StartFrame == Clips[j].StartFrame && Clips[i].EndFrame == Clips[j].EndFrame)
				Clips.RemoveAt(i);
			else
				Clips[i].Name += FString::FromInt(i);
		}
	}
}
bool SFBXBatchImportWin::DoSplit(UAnimSequence* AnimSequence, const TArray<FClipInfo> Clips, const FString& SaveDir, TArray<UObject*>& Assets, float Progress)
{
	struct TrackFrames
	{
		FName				BoneName;
		TArray<FTransform>	Trans;
	};



	FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");

	TArray<UObject*> NewAssets;
	const int32 SourceFrames = AnimSequence->GetNumberOfSampledKeys();
	IAnimationDataController& SrcAniCtrl = AnimSequence->GetController();
	IAnimationDataModel* SrcModelData = AnimSequence->GetDataModel();

	auto FrameRate = AnimSequence->GetSamplingFrameRate();

	TArray<FName> TrackNames;
	TArray<TrackFrames> TrackTrans;
	SrcModelData->GetBoneTrackNames(TrackNames);

	for (FName& Name : TrackNames)
	{
		TrackFrames& Frames = TrackTrans.AddDefaulted_GetRef();
		Frames.BoneName = Name;
		SrcModelData->GetBoneTrackTransforms(Name, Frames.Trans);
	}

	auto Factory = NewObject<UAnimSequenceFactory>();
	Factory->TargetSkeleton = AnimSequence->GetSkeleton();

	for (const auto& Clip : Clips)
	{
		const int32 StartFrame = FMath::Clamp(Clip.StartFrame, 0, SourceFrames - 1);
		const int32 EndFrame = FMath::Clamp(Clip.EndFrame, StartFrame, SourceFrames - 1);

		FString NewName = FString::Printf(TEXT("%s_%s"),
			*AnimSequence->GetName(),
			*Clip.Name);

		//FString AssetPath = FPaths::Combine(SaveDir, NewName);
		//if (UEditorAssetLibrary::DoesAssetExist(AssetPath))
		//	DeleteAssetPackage(AssetPath);

		UAnimSequence* NewAnim = Cast<UAnimSequence>(AssetToolsModule.Get().CreateAsset(
			NewName,
			SaveDir,
			UAnimSequence::StaticClass(),
			Factory));

		NewAnim->MarkPackageDirty();

		m_ProgressWindow->UpdateProgress(Progress, FText::Format(LOCTEXT("BuildClip", "BuildClip: {0}"), FText::FromString(FPaths::Combine(SaveDir, NewName))).ToString());

		IAnimationDataController& NewAniCtrl = NewAnim->GetController();
		IAnimationDataModel* NewModelData = NewAnim->GetDataModel();

		NewAniCtrl.SetFrameRate(FrameRate, true);
		NewAniCtrl.SetNumberOfFrames(EndFrame - StartFrame + 1);

		for (TrackFrames& Track : TrackTrans)
		{
			TArray<FVector> PositionalKeys;
			TArray<FQuat> RotationalKeys;
			TArray<FVector> ScalingKeys;
			PositionalKeys.Reserve(EndFrame - StartFrame + 1);
			RotationalKeys.Reserve(EndFrame - StartFrame + 1);
			ScalingKeys.Reserve(EndFrame - StartFrame + 1);
			for (int i = StartFrame; i <= EndFrame && i < Track.Trans.Num(); i++)
			{
				FTransform& Trans = Track.Trans[i];
				PositionalKeys.Push(Trans.GetTranslation());
				RotationalKeys.Push(Trans.GetRotation());
				ScalingKeys.Push(Trans.GetScale3D());
			}
			NewAniCtrl.AddBoneCurve(Track.BoneName);
			NewAniCtrl.SetBoneTrackKeys(Track.BoneName, PositionalKeys, RotationalKeys, ScalingKeys);
		}

		NewModelData->GetModifiedEvent().Broadcast(EAnimDataModelNotifyType::Populated, NewModelData, FAnimDataModelNotifPayload());

		NewAnim->PostEditChange();

		FAssetRegistryModule::AssetCreated(NewAnim);
		NewAssets.Add(NewAnim);
		Assets.Add(NewAnim);
	}

	FContentBrowserModule& ContentBrowserModule = FModuleManager::GetModuleChecked<FContentBrowserModule>("ContentBrowser");
	ContentBrowserModule.Get().SyncBrowserToAssets(NewAssets);

	return true;
}

void SFBXBatchImportWin::DeleteAssetPackage(const FString& PackagePath)
{
	UObject* Asset = LoadObject<UObject>(nullptr, *PackagePath);

	if (Asset)
	{
		Asset->ClearFlags(RF_Public | RF_Standalone);
		Asset->RemoveFromRoot();
		Asset->MarkAsGarbage();
	}

	if (UEditorAssetLibrary::DoesAssetExist(PackagePath))
	{
		UEditorAssetLibrary::DeleteAsset(PackagePath);
	}

	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
}

#undef LOCTEXT_NAMESPACE
