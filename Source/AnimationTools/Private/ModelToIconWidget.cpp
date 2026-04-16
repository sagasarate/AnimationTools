// Fill out your copyright notice in the Description page of Project Settings.

#include "ModelToIconWidget.h"
#include "Widgets/SWindow.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "EditorUtilitySubsystem.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "ImageUtils.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"

#define LOCTEXT_NAMESPACE "ModelToIcon"

void UModelToIconWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UModelToIconWidget::RenderTick));
}

void UModelToIconWidget::NativeDestruct()
{
	if (NotificationItem.IsValid())
	{
		NotificationItem->ExpireAndFadeout();
		NotificationItem.Reset();
	}
	FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
	Super::NativeDestruct();
}

// void UModelToIconWidget::OpenAsWindow()
// {
//     FString Path = TEXT("/AnimationTools/UI_ModelToIcon.UI_ModelToIcon");
// 	UEditorUtilityWidgetBlueprint* BP =
// 		LoadObject<UEditorUtilityWidgetBlueprint>(
// 			nullptr,
// 			Path);
// 	if (!BP)
// 	{
// 		UE_LOG(LogTemp, Error, TEXT("Failed to load widget for path %s"), *Path);
// 		return;
// 	}
// 	UEditorUtilitySubsystem* Subsystem =
// 		GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();

// 	Subsystem->SpawnAndRegisterTab(BP);
// }

void UModelToIconWidget::OnSelectInputFolder()
{
	// Open Content Browser path picker to select an input folder
	if (!IsValid(txModelFolder))
	{
		UE_LOG(LogTemp, Warning, TEXT("txModelFolder is not valid"));
		return;
	}
	FPathPickerConfig PickerConfig;
	PickerConfig.DefaultPath = txModelFolder->GetText().ToString();
	PickerConfig.bNotifyDefaultPathSelected = true;
	PickerConfig.OnPathSelected = FOnPathSelected::CreateWeakLambda(this, [this](const FString& InPath) {
		if (IsValid(txModelFolder))
		{
			txModelFolder->SetText(FText::FromString(InPath));
		}
	});

	TSharedRef<SWidget> PathPickerWidget = IContentBrowserSingleton::Get().CreatePathPicker(PickerConfig);

	TSharedRef<SWindow> PickerWindow = SNew(SWindow)
										   .Title(LOCTEXT("SelectInputFolderWindow", "Select Input Folder"))
										   .ClientSize(FVector2D(600, 400))
											   [PathPickerWidget];

	AddModalSubWindow(PickerWindow);
}

void UModelToIconWidget::OnSelectOutputFolder()
{
	// Open Content Browser path picker to select an output folder
	if (!IsValid(txOutputlFolder))
	{
		UE_LOG(LogTemp, Warning, TEXT("txOutputlFolder is not valid"));
		return;
	}
	FPathPickerConfig PickerConfig;
	PickerConfig.DefaultPath = txOutputlFolder->GetText().ToString();
	PickerConfig.bNotifyDefaultPathSelected = true;
	PickerConfig.OnPathSelected = FOnPathSelected::CreateWeakLambda(this, [this](const FString& InPath) {
		if (IsValid(txOutputlFolder))
		{
			txOutputlFolder->SetText(FText::FromString(InPath));
		}
	});

	TSharedRef<SWidget> PathPickerWidget = IContentBrowserSingleton::Get().CreatePathPicker(PickerConfig);

	TSharedRef<SWindow> PickerWindow = SNew(SWindow)
										   .Title(LOCTEXT("SelectOutputFolderWindow", "Select Output Folder"))
										   .ClientSize(FVector2D(600, 400))
											   [PathPickerWidget];
	AddModalSubWindow(PickerWindow);
}

void UModelToIconWidget::AddModalSubWindow(TSharedRef<SWindow> SubWindow)
{
	TSharedPtr<SWidget> SafeWidget = GetCachedWidget(); // 或者使用 TakeWidget()，但建议 GetCachedWidget

	if (SafeWidget.IsValid())
	{
		// 在 Slate 树中向上查找最近的 Window
		TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(SafeWidget.ToSharedRef());

		if (ParentWindow.IsValid())
		{
			// 将 PickerWindow 挂载到查找到的父窗口上
			FSlateApplication::Get().AddModalWindow(SubWindow, ParentWindow.ToSharedRef());
		}
		else
		{
			// 如果找不到父窗口（比如还没显示），就直接弹出独立窗口
			FSlateApplication::Get().AddWindow(SubWindow);
		}
	}
}

void UModelToIconWidget::OnGenerateIcons()
{
	FetchRenderParams();

	// Clear any previous previews
	tlvPreviewListView->ClearListItems();

	FString ModelPath = txModelFolder->GetText().ToString();
	if (IsValid(txModelFolder))
	{
		ModelPath = txModelFolder->GetText().ToString();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("txModelFolder is not valid"));
	}

	if (ModelPath.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Model folder path is empty"));
		return;
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	FARFilter			  Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(FName(*ModelPath));
	// Use ClassPaths filter (preferred API)
	Filter.ClassPaths.Add(UStaticMesh::StaticClass()->GetClassPathName());

	TArray<FAssetData> MeshAssets;
	AssetRegistryModule.Get().GetAssets(Filter, MeshAssets);
	if (MeshAssets.Num() > 0)
	{
		for (auto Asset : MeshAssets)
		{
			if (Asset.AssetClassPath == UStaticMesh::StaticClass()->GetClassPathName())
			{
				RenderTaskList.Add(RenderTaskInfo(TSoftObjectPtr<UStaticMesh>(Asset.ToSoftObjectPath()), nullptr));
			}
		}

		TotalRenderProgress = RenderTaskList.Num();
		FNotificationInfo Info(LOCTEXT("ModelToIcon", "Meshes rendering..."));
		Info.bFireAndForget = false;
		Info.FadeOutDuration = 1.0f;
		Info.ExpireDuration = 0.0f;
		NotificationItem = FSlateNotificationManager::Get().AddNotification(Info);
		if (NotificationItem.IsValid())
		{
			NotificationItem->SetCompletionState(SNotificationItem::CS_Pending);
		}
	}
}
void UModelToIconWidget::OnReGenerateSelected()
{
	if (!IsValid(tlvPreviewListView))
	{
		UE_LOG(LogTemp, Warning, TEXT("tlvPreviewListView is not valid"));
		return;
	}
	TArray<UObject*> SelectedItems;
	tlvPreviewListView->GetSelectedItems(SelectedItems);
	FetchRenderParams();
	for (auto Obj : SelectedItems)
	{
		UModelToIconPreviewData* Data = Cast<UModelToIconPreviewData>(Obj);
		if (Data && Data->SoftMesh.IsValid())
		{
			RenderTaskList.Add(RenderTaskInfo(Data->SoftMesh, Data));
		}
	}
	if (RenderTaskList.Num() > 0)
	{
		TotalRenderProgress = RenderTaskList.Num();
		FNotificationInfo Info(LOCTEXT("ModelToIcon", "Meshes rendering..."));
		Info.bFireAndForget = false;
		Info.FadeOutDuration = 1.0f;
		Info.ExpireDuration = 0.0f;
		NotificationItem = FSlateNotificationManager::Get().AddNotification(Info);
		if (NotificationItem.IsValid())
		{
			NotificationItem->SetCompletionState(SNotificationItem::CS_Pending);
		}
	}
}

void UModelToIconWidget::OnSaveIcons()
{
	int32	SaveCount = 0;
	FString PackagePath;
	if (IsValid(txOutputlFolder))
	{
		PackagePath = txOutputlFolder->GetText().ToString();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("txOutputlFolder is not valid"));
	}

	if (PackagePath.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Output folder path is empty"));
		return;
	}
	if (PackagePath.EndsWith(TEXT("/")))
	{
		PackagePath.LeftChopInline(1);
	}
	auto& ListItems = tlvPreviewListView->GetListItems();
	for (auto Item : ListItems)
	{
		UModelToIconPreviewData* Data = Cast<UModelToIconPreviewData>(Item);
		if (!IsValid(Data) || !Data->bExport || !Data->IconTexture.IsValid())
		{
			continue;
		}

		// 1. 规范化资产名称
		FString BaseName = Data->IconName;
		for (TCHAR& Ch : BaseName)
		{
			if (!(FChar::IsAlnum(Ch) || Ch == '_'))
			{
				Ch = '_';
			}
		}

		FString SanitizedPackageName = PackagePath / BaseName;
		if (!SanitizedPackageName.StartsWith(TEXT("/")))
		{
			SanitizedPackageName = TEXT("/") + SanitizedPackageName;
		}

		// 2. 检测资产是否已存在
		if (FPackageName::DoesPackageExist(SanitizedPackageName))
		{
			FText ConfirmMsg = FText::Format(
				NSLOCTEXT("SModelToIcon", "OverwriteConfirm", "资产 {0} 已存在，是否覆盖？"),
				FText::FromString(BaseName));

			// 弹出对话框 (Yes/No)
			EAppReturnType::Type Res = FMessageDialog::Open(EAppMsgType::YesNo, ConfirmMsg);
			if (Res == EAppReturnType::No)
			{
				UE_LOG(LogTemp, Log, TEXT("Skipped overwriting %s"), *SanitizedPackageName);
				continue;
			}
		}

		// 3. 创建或获取 Package
		// 注意：如果已存在，TryLoadPackage 可以确保我们在原有的 Package 上操作，避免重名冲突
		UPackage* Package = CreatePackage(*SanitizedPackageName);
		if (!Package)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create package for %s"), *SanitizedPackageName);
			continue;
		}
		Package->FullyLoad(); // 确保加载完整以防覆盖时出错

		UTexture2D* SourceTex = Data->IconTexture.Get();

		// 4. 创建或查找 Texture 对象 (如果已存在则覆盖)
		UTexture2D* NewTex = FindObject<UTexture2D>(Package, *BaseName);
		if (!NewTex)
		{
			NewTex = NewObject<UTexture2D>(Package, *BaseName, RF_Public | RF_Standalone);
		}

		if (!NewTex)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create/find texture asset for %s"), *BaseName);
			continue;
		}

		// 5. 初始化贴图数据 (复用原有的逻辑)
		if (Data->ImageData.IsValid())
		{
			FImage& Img = *Data->ImageData;
			NewTex->Source.Init(Img.SizeX, Img.SizeY, 1, 1, TSF_BGRA8);
			uint8* Dest = NewTex->Source.LockMip(0);
			FMemory::Memcpy(Dest, Img.RawData.GetData(), Img.RawData.Num());
			NewTex->Source.UnlockMip(0);
			NewTex->SRGB = true;
			NewTex->MipGenSettings = TMGS_NoMipmaps;
		}
		else if (SourceTex && SourceTex->Source.GetSizeX() > 0)
		{
			int32 SX = SourceTex->Source.GetSizeX();
			int32 SY = SourceTex->Source.GetSizeY();
			NewTex->Source.Init(SX, SY, 1, 1, TSF_BGRA8);
			uint8*		 Dest = NewTex->Source.LockMip(0);
			const uint8* Src = SourceTex->Source.LockMipReadOnly(0);
			int32		 SrcSize = SourceTex->Source.CalcMipSize(0);
			FMemory::Memcpy(Dest, Src, SrcSize);
			SourceTex->Source.UnlockMip(0);
			NewTex->Source.UnlockMip(0);
			NewTex->SRGB = SourceTex->SRGB;
		}
		else
		{
			continue;
		}

		// 6. 保存到磁盘
		NewTex->UpdateResource();
		Package->MarkPackageDirty();
		FAssetRegistryModule::AssetCreated(NewTex);

		FString			 PackageFileName = FPackageName::LongPackageNameToFilename(SanitizedPackageName, FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;

		if (UPackage::SavePackage(Package, NewTex, *PackageFileName, SaveArgs))
		{
			SaveCount++;
			UE_LOG(LogTemp, Log, TEXT("Saved icon asset %s to %s"), *BaseName, *PackageFileName);
		}
	}

	// 完成后弹出通知
	FText FinalMsg = FText::Format(NSLOCTEXT("SModelToIcon", "SaveComplete", "成功保存了 {0} 个图标"), FText::AsNumber(SaveCount));
	FMessageDialog::Open(EAppMsgType::Ok, FinalMsg);
}

bool UModelToIconWidget::RenderTick(float DeltaTime)
{
	if (!MeshRenderer.IsValid())
	{
		MeshRenderer = MakeUnique<FMeshRenderer>();
	}
	auto State = MeshRenderer->TickRender(DeltaTime);
	if (State == FMeshRenderer::ERenderState::Completed)
	{
		auto&	Image = MeshRenderer->GetOutputImage();
		FString IconName = MeshRenderer->GetSourceName();
		for (TCHAR& Ch : IconName)
		{
			if (!(FChar::IsAlnum(Ch) || Ch == '_'))
			{
				Ch = '_';
			}
		}
		UModelToIconPreviewData* ItemData = Cast<UModelToIconPreviewData>(MeshRenderer->GetParam());
		if (IsValid(ItemData))
		{
			ItemData->ImageData = MakeShared<FImage>(Image);
			ItemData->IconTexture = FImageUtils::CreateTexture2DFromImage(*ItemData->ImageData.Get());
			tlvPreviewListView->RegenerateAllEntries();
		}
		else
		{
			ItemData = NewObject<UModelToIconPreviewData>();
			ItemData->ImageData = MakeShared<FImage>(Image);
			ItemData->SoftMesh = MeshRenderer->GetSoftMeshPtr();
			ItemData->IconTexture = FImageUtils::CreateTexture2DFromImage(*ItemData->ImageData.Get());
			ItemData->IconName = IconName;
			tlvPreviewListView->AddItem(ItemData);
		}
		MeshRenderer->Reset();
	}
	if (State == FMeshRenderer::ERenderState::None || State == FMeshRenderer::ERenderState::Completed || State == FMeshRenderer::ERenderState::Error)
	{
		if (RenderTaskList.Num() > 0)
		{
			RenderTaskInfo& TaskInfo = RenderTaskList[0];
			MeshRenderer->Render(TaskInfo.SoftMeshPtr, CurrentIconSize, (float)MeshRotationDeg, (float)CameraAxisRotationDeg, (float)CameraViewAngleDeg, CameraFOVDeg);
			MeshRenderer->SetParam(TaskInfo.ItemData);
			RenderTaskList.RemoveAt(0);
			if (NotificationItem.IsValid())
			{
				FText ProgressText = FText::Format(LOCTEXT("ModelToIconProgress", "Meshes rendering... {0} remaining"), FText::AsNumber(RenderTaskList.Num()));
				NotificationItem->SetText(ProgressText);
			}
		}
		else if (TotalRenderProgress > 0)
		{
			TotalRenderProgress = 0;
			if (NotificationItem.IsValid())
			{
				FText CompleteText = LOCTEXT("ModelToIconComplete", "Mesh rendering complete");
				NotificationItem->SetText(CompleteText);
				NotificationItem->SetCompletionState(SNotificationItem::CS_Success);
				NotificationItem->ExpireAndFadeout();
				NotificationItem.Reset();
			}
		}
	}
	return true;
}

void UModelToIconWidget::FetchRenderParams()
{
	if (IsValid(sbIconSize))
	{
		CurrentIconSize = sbIconSize->GetValue();
	}
	if (IsValid(sbMeshRotation))
	{
		MeshRotationDeg = sbMeshRotation->GetValue();
	}
	if (IsValid(sbViewRotation))
	{
		CameraAxisRotationDeg = sbViewRotation->GetValue();
	}
	if (IsValid(sbViewAngle))
	{
		CameraViewAngleDeg = sbViewAngle->GetValue();
	}
	if (IsValid(sbCameraFOV))
	{
		CameraFOVDeg = sbCameraFOV->GetValue();
	}
}