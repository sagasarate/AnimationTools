#include "AnimationSplitWindow.h"
#include "AnimationTools.h"

#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Animation/AnimSequence.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "Containers/StringFwd.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "MovieScene.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Factories/AnimSequenceFactory.h"
#include "Animation/AnimCompress.h"

#include "SlateIconBrowser.h"


#define LOCTEXT_NAMESPACE "AnimationTools"

void SAnimationSplitWindow::Construct(const FArguments& InArgs)
{
	FVector2D ButtonSize = CalculateButtonSize("Icons.Delete", "HoverHintOnly", FMargin(0));

	SWindow::Construct(SWindow::FArguments()
		.Title(LOCTEXT("WindowTitle", "Animation Splitter"))
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
						+ SVerticalBox::Slot().AutoHeight().Padding(5)
						[
							SNew(SButton)
								.Text(LOCTEXT("AddEntry", "Add New Entry"))
								.HAlign(HAlign_Center)
								.OnClicked_Lambda([this]() {
								AddEntry();
								return FReply::Handled();
									})
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(2)
						[
							SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().FillWidth(0.4f)
								[
									SNew(SBorder)
										.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
										.BorderBackgroundColor(FAppStyle::Get().GetColor("Colors.Panel"))
										[
											SNew(STextBlock)
												.Text(LOCTEXT("EntryName", "Name"))
										]
								]
								+ SHorizontalBox::Slot().FillWidth(0.2f).Padding(2, 0)
								[
									SNew(SBorder)
										.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
										.BorderBackgroundColor(FAppStyle::Get().GetColor("Colors.Panel"))
										[
											SNew(STextBlock)
												.Text(LOCTEXT("EntryStartFrame", "Start Frame"))
										]
								]
								+ SHorizontalBox::Slot().FillWidth(0.2f).Padding(2, 0)
								[
									SNew(SBorder)
										.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
										.BorderBackgroundColor(FAppStyle::Get().GetColor("Colors.Panel"))
										[
											SNew(STextBlock)
												.Text(LOCTEXT("EntryEndFrame", "End Frame"))
										]
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0, 0, 0)
								[
									SNew(SBox).WidthOverride(ButtonSize.X)
								]

						]
						+ SVerticalBox::Slot().FillHeight(1.0f)
						[
							SNew(SScrollBox)
								+ SScrollBox::Slot()
								[
									SAssignNew(EntriesBox, SVerticalBox)
								]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(5)
						[
							SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth()
								[
									SNew(STextBlock)
										.Text(LOCTEXT("SaveToDir", "Save to directory:"))
								]
								+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2)
								[
									SAssignNew(SaveDirTextBox, SEditableTextBox)
										.Text(FText::FromString(SaveDir))
										.OnTextChanged_Lambda([this](const FText& Text) {SaveDir = Text.ToString(); })
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(2)
								[
									SNew(SButton)
										.Text(FText::FromString("..."))
										.OnClicked(this, &SAnimationSplitWindow::OnSaveDirClicked)
								]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(5)
						[
							SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().FillWidth(1.0f)
								[
									SNew(SButton)
										.Text(LOCTEXT("SplitButton", "Split Animations"))
										.HAlign(HAlign_Center)
										.OnClicked(this, &SAnimationSplitWindow::OnSplitClicked)
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(5, 0, 0, 0)
								[
									SNew(SButton)
										.Text(LOCTEXT("OpenCSVButton", "Import from CSV"))
										.OnClicked_Lambda([this]() {
										ImportFromCSV();
										return FReply::Handled();
											})
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(5, 0, 0, 0)
								[
									SNew(SButton)
										.Text(LOCTEXT("SaveCSVButton", "Export to CSV"))
										.OnClicked_Lambda([this]() {
										ExportToCSV();
										return FReply::Handled();
											})
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(5, 0, 0, 0)
								[
									SNew(SButton)
										.Text(LOCTEXT("CloseButton", "Close"))
										.ToolTipText(LOCTEXT("CloseTooltip", "Close this window"))
										.OnClicked_Lambda([this]() {
										RequestDestroyWindow();
										return FReply::Handled();
											})
								]
						]
				]
		]);

		
}

void SAnimationSplitWindow::OpenWindow(UAnimSequence* Anim)
{
	TSharedRef<SAnimationSplitWindow> Window = SNew(SAnimationSplitWindow);
	Window->SelectedAnimSequence = Anim;
	FSlateApplication::Get().AddWindow(Window);
	if (Anim)
	{
		Window->OnSaveDirSelected(FPaths::GetPath(Anim->GetPathName()) + "/Anims");
	}
	Window->AddEntry();

	//TSharedRef<SSlateIconBrowser> Window2 = SNew(SSlateIconBrowser);
	//FSlateApplication::Get().AddWindow(Window2);
}

FVector2D SAnimationSplitWindow::CalculateButtonSize(FName IconName, FName ButtonStyleName, FMargin ExtraPadding)
{
	// 获取图标尺寸
	const FSlateBrush* IconBrush = FAppStyle::Get().GetBrush(IconName);
	FVector2D IconSize = IconBrush ? IconBrush->ImageSize : FVector2D(16, 16);

	// 获取样式内边距
	const FButtonStyle& ButtonStyle = FAppStyle::Get().GetWidgetStyle<FButtonStyle>(ButtonStyleName);
	FMargin StylePadding = ButtonStyle.Hovered.GetMargin();

	// 计算总尺寸
	return FVector2D(
		IconSize.X + IconSize.X * StylePadding.Left + IconSize.X * StylePadding.Right + ExtraPadding.Left + ExtraPadding.Right,
		IconSize.Y + IconSize.Y * StylePadding.Top + IconSize.Y * StylePadding.Bottom + ExtraPadding.Top + ExtraPadding.Bottom
	);
}


void SAnimationSplitWindow::AddEntry(const FString& Name, int32 StartFrame, int32 EndFrame, bool RefreshShow)
{
	TSharedPtr<FAnimSplitEntry> NewEntry = MakeShared<FAnimSplitEntry>();
	NewEntry->SequenceName = Name;
	NewEntry->StartFrame = StartFrame;
	NewEntry->EndFrame = EndFrame;

	// 创建删除按钮
	NewEntry->DeleteButton = SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
		.ToolTipText(LOCTEXT("DeleteEntryTooltip", "Delete this entry"))
		.OnClicked_Lambda([this, NewEntry]() {
		if (SplitEntries.Contains(NewEntry))
		{
			if (FMessageDialog::Open(EAppMsgType::YesNo,
				LOCTEXT("DeleteConfirm", "Are you sure to delete this entry?")) == EAppReturnType::Yes)
			{
				// 使用RemoveSwap避免破坏迭代器
				SplitEntries.RemoveSwap(NewEntry);
				// 立即重建
				RebuildEntriesBox();
			}
		}
		return FReply::Handled();
			})
		[
			SNew(SImage)
				.Image(FAppStyle::GetBrush("Icons.Delete"))
				.ColorAndOpacity(FSlateColor::UseForeground())
		];
		

	SplitEntries.Add(NewEntry);
	if (RefreshShow)
		RebuildEntriesBox();
}

void SAnimationSplitWindow::RebuildEntriesBox()
{
	EntriesBox->ClearChildren();
	if (!SelectedAnimSequence.Get())
		return;


	int32 MaxFrame = SelectedAnimSequence->GetNumberOfSampledKeys() - 1;
	TArray<TSharedPtr<FAnimSplitEntry>> ValidEntries = SplitEntries;
	for (auto& Entry : ValidEntries)
	{
		EntriesBox->AddSlot()
			.AutoHeight()
			.Padding(2)
			[
				SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(0.4f)
					[
						SNew(SEditableTextBox)
							.Text(FText::FromString(Entry->SequenceName))
							.OnTextChanged_Lambda([Entry](const FText& Text) {Entry->SequenceName = Text.ToString(); })
					]
					+ SHorizontalBox::Slot().FillWidth(0.2f).Padding(2, 0)
					[
						SNew(SSpinBox<int32>)
							.Value_Lambda([Entry] {return Entry->StartFrame; })
							.MinValue(0)
							.MaxValue(MaxFrame)
							.OnValueChanged_Lambda([Entry](int32 Value) {Entry->StartFrame = Value; })
					]
					+ SHorizontalBox::Slot().FillWidth(0.2f).Padding(2, 0)
					[
						SNew(SSpinBox<int32>)
							.Value_Lambda([Entry] {return Entry->EndFrame; })
							.MinValue(0)
							.MaxValue(MaxFrame)
							.OnValueChanged_Lambda([Entry](int32 Value) {Entry->EndFrame = Value; })
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0, 0, 0)
					[
						Entry->DeleteButton.ToSharedRef()
					]
			];
	}
	// 强制刷新Slate布局
	Invalidate(EInvalidateWidgetReason::Layout);
}

FReply SAnimationSplitWindow::OnSplitClicked()
{
	struct TrackFrames
	{
		FName				BoneName;
		TArray<FTransform>	Trans;
	};
	if (!ValidateEntries())
		return FReply::Handled();

	FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");

	TArray<UObject*> NewAssets;
	const int32 SourceFrames = SelectedAnimSequence->GetNumberOfSampledKeys();
	IAnimationDataController& SrcAniCtrl = SelectedAnimSequence->GetController();
	IAnimationDataModel* SrcModelData = SelectedAnimSequence->GetDataModel();

	auto FrameRate = SelectedAnimSequence->GetSamplingFrameRate();

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
	Factory->TargetSkeleton = SelectedAnimSequence->GetSkeleton();

	for (const auto& Entry : SplitEntries)
	{
		const int32 StartFrame = FMath::Clamp(Entry->StartFrame, 0, SourceFrames - 1);
		const int32 EndFrame = FMath::Clamp(Entry->EndFrame, StartFrame, SourceFrames - 1);

		FString NewName = FString::Printf(TEXT("%s_%s"),
			*SelectedAnimSequence->GetName(),
			*Entry->SequenceName);

		UAnimSequence* NewAnim = Cast<UAnimSequence>(AssetToolsModule.Get().CreateAsset(
			NewName,
			SaveDir,
			UAnimSequence::StaticClass(),
			Factory));

		NewAnim->MarkPackageDirty();

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
			//NewAniCtrl.AddBoneTrack(Track.BoneName);
			NewAniCtrl.AddBoneCurve(Track.BoneName);
			NewAniCtrl.SetBoneTrackKeys(Track.BoneName, PositionalKeys, RotationalKeys, ScalingKeys);
		}

		NewModelData->GetModifiedEvent().Broadcast(EAnimDataModelNotifyType::Populated, NewModelData, FAnimDataModelNotifPayload());

		//TSharedPtr<FAnimCompressContext> CompressContext = MakeShareable(new FAnimCompressContext(false, false));

		//NewAnim->RequestAnimCompression(FRequestAnimCompressionParams(false, CompressContext));
		//NewAnim->CacheDerivedData();
		NewAnim->PostEditChange();

		FAssetRegistryModule::AssetCreated(NewAnim);
		NewAssets.Add(NewAnim);
	}

	FContentBrowserModule& ContentBrowserModule = FModuleManager::GetModuleChecked<FContentBrowserModule>("ContentBrowser");
	ContentBrowserModule.Get().SyncBrowserToAssets(NewAssets);

	this->RequestDestroyWindow();
	return FReply::Handled();
}

FReply SAnimationSplitWindow::OnSaveDirClicked()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	FPathPickerConfig FolderDialogConfig;
	FolderDialogConfig.DefaultPath = SaveDir;
	FolderDialogConfig.bAddDefaultPath = true;
	FolderDialogConfig.OnPathSelected.BindRaw(this, &SAnimationSplitWindow::OnSaveDirSelected);

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

void SAnimationSplitWindow::OnSaveDirSelected(const FString& SelectDir)
{
	SaveDir = SelectDir;
	SaveDirTextBox->SetText(FText::FromString(SaveDir));
}

bool SAnimationSplitWindow::ValidateEntries() const
{
	if (!SelectedAnimSequence.Get())
	{
		FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok,
			LOCTEXT("ErrorMsg.AnimError", "No selected Animation"));
		return false;
	}

	const int32 TotalFrames = SelectedAnimSequence->GetNumberOfSampledKeys();
	for (const auto& Entry : SplitEntries)
	{
		if (Entry->SequenceName.IsEmpty())
		{
			FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok,
				LOCTEXT("ErrorMsg.NameError", "Animation clip name is empty"));
			return false;
		}
		if (Entry->StartFrame < 0 || Entry->StartFrame >= TotalFrames)
		{
			FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok,
				LOCTEXT("ErrorMsg.FrameError1", "Invalid start frame"));
			return false;
		}
		if (Entry->EndFrame < 0 || Entry->EndFrame >= TotalFrames)
		{
			FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok,
				LOCTEXT("ErrorMsg.FrameError2", "Invalid end frame"));
			return false;
		}
		if (Entry->StartFrame > Entry->EndFrame)
		{
			FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok,
				LOCTEXT("ErrorMsg.FrameError3", "The end frame must be greater than or equal to the start frame"));
			return false;
		}
	}
	return true;
}



void SAnimationSplitWindow::ImportFromCSV()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		TArray<FString> OutFiles;
		const FText DlgTitle = LOCTEXT("OpenDialog.Title", "Open File");
		const FString DefaultPath = FPaths::ProjectContentDir();
		const FText FileTypes = LOCTEXT("OpenDialog.FileFilter", "All Files (*.*)|*.*|CSV Files (*.csv)|*.csv");

		uint32 SelectionFlag = 0; // 0 = 单选，1 = 多选

		DesktopPlatform->OpenFileDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			DlgTitle.ToString(),
			DefaultPath,
			TEXT(""),
			FileTypes.ToString(),
			SelectionFlag,
			OutFiles
		);

		if (OutFiles.Num() > 0)
		{
			const FString SelectedFile = OutFiles[0];
			// 处理选择的文件

			FString FileContent;
			if (FFileHelper::LoadFileToString(FileContent, *SelectedFile))
			{
				StringToEntries(FileContent);
			}
			else
			{
				FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok,
					FText::Format(LOCTEXT("ErrorMsg.CSVOpenFailed", "Can`t open CSV file:{0}"), FText::FromString(SelectedFile)));
			}
		}
	}
}
void SAnimationSplitWindow::ExportToCSV()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform && SelectedAnimSequence.Get())
	{
		TArray<FString> OutFiles;
		const FText DlgTitle = LOCTEXT("SaveDialog.Title", "Save File");
		const FString DefaultPath = FPaths::ProjectSavedDir();
		const FText FileTypes = LOCTEXT("SaveDialog.FileFilter", "CSV Files (*.csv)|*.csv");
		const FString DefaultFile = FString::Printf(TEXT("%s.csv"), *SelectedAnimSequence->GetName());

		DesktopPlatform->SaveFileDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			DlgTitle.ToString(),
			DefaultPath,
			DefaultFile,
			FileTypes.ToString(),
			0, // 保存选项（保留参数）
			OutFiles
		);

		if (OutFiles.Num() > 0)
		{
			const FString SavePath = OutFiles[0];
			// 处理保存路径

			FStringBuilderBase StringBuilder;
			StringBuilder.Append(TEXT("Name,StartFrame,EndFrame\n"));
			for (auto& Entry : SplitEntries)
			{
				StringBuilder.Appendf(TEXT("%s,%d,%d\n"), *Entry->SequenceName, Entry->StartFrame, Entry->EndFrame);
			}

			if (FFileHelper::SaveStringToFile(StringBuilder.ToString(), *SavePath, FFileHelper::EEncodingOptions::AutoDetect))
			{
				FMessageDialog::Open(EAppMsgCategory::Success, EAppMsgType::Ok,
					FText::Format(LOCTEXT("ErrorMsg.CSVSaveSucceed", "Succeed export to:{0}"), FText::FromString(SavePath)));
			}
			else
			{
				FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok,
					FText::Format(LOCTEXT("ErrorMsg.CSVSaveFailed", "Can`t save CSV file:{0}"), FText::FromString(SavePath)));
			}
		}
	}
}


bool SAnimationSplitWindow::StringToEntries(FString& Str)
{
	TArray<FString> Rows;
	Str.ParseIntoArrayLines(Rows);

	int NameIndex = INDEX_NONE;
	int StartFrameIndex = INDEX_NONE;
	int EndFrameIndex = INDEX_NONE;
	for (const FString& Row : Rows)
	{
		TArray<FString> Columns;
		Row.ParseIntoArray(Columns, TEXT(","));

		if (NameIndex < 0)
		{
			if (Columns.Num() >= 3)
			{
				NameIndex = Columns.IndexOfByKey(TEXT("Name"));
				StartFrameIndex = Columns.IndexOfByKey(TEXT("StartFrame"));
				EndFrameIndex = Columns.IndexOfByKey(TEXT("EndFrame"));
				if (NameIndex == INDEX_NONE || StartFrameIndex == INDEX_NONE || EndFrameIndex == INDEX_NONE)
				{
					FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok,
						LOCTEXT("ErrorMsg.CSVFormatError", "Invalid CSV format"));
					return false;
				}
			}
			else
			{
				FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok,
					LOCTEXT("ErrorMsg.CSVFormatError", "Invalid CSV format"));
				return false;
			}
			SplitEntries.Empty();
		}
		else
		{
			FString Name;
			int32 StartFrame = 0;
			int32 EndFrame = 0;
			if (NameIndex >= 0)
				Name = Columns[NameIndex].TrimStartAndEnd();
			if (StartFrameIndex >= 0)
				StartFrame = FCString::Atoi(*Columns[StartFrameIndex]);
			if (EndFrameIndex >= 0)
				EndFrame = FCString::Atoi(*Columns[EndFrameIndex]);
			AddEntry(Name, StartFrame, EndFrame, false);
		}
	}
	if (NameIndex >= 0)
	{
		RebuildEntriesBox();
		return true;
	}
	else
	{
		FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Ok,
			LOCTEXT("ErrorMsg.CSVFormatError", "Invalid CSV format"));
		return false;
	}
}

#undef LOCTEXT_NAMESPACE
