// Fill out your copyright notice in the Description page of Project Settings.

#include "MeshLODSettingWidget.h"
#include "Widgets/SWindow.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "EditorUtilitySubsystem.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Engine/StaticMesh.h"
#include "Misc/MessageDialog.h"

#define LOCTEXT_NAMESPACE "ModelToIcon"

void UMeshLODSettingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 初始化默认 LOD 分档规则
	const struct
	{
		int32			MaxTriangle;
		TArray<float>	Redcudes;
	} Defaults[] = {
		{ 1000, { 1.0f } },
		{ 10000, { 1.0f, 0.5f } },
		{ 50000, { 1.0f, 0.5f, 0.25f, 0.12f } },
		{ 100000, { 1.0f, 0.5f, 0.25f, 0.12f, 0.06f, 0.03f } },
	};
	if (IsValid(lvLODSettings) && lvLODSettings->GetListItems().Num() == 0)
	{
		for (const auto& Def : Defaults)
		{
			auto Item = NewObject<UMeshLODSettingListItemData>(this);
			Item->MaxTriangle = Def.MaxTriangle;
			Item->LODRedcudes = Def.Redcudes;
			lvLODSettings->AddItem(Item);
		}
	}
}

void UMeshLODSettingWidget::NativeDestruct()
{
	Super::NativeDestruct();
}

void UMeshLODSettingWidget::OnSelectMeshFolder()
{
	// Open Content Browser path picker to select a mesh folder
	if (!IsValid(txMeshFolder))
	{
		UE_LOG(LogTemp, Warning, TEXT("txMeshFolder is not valid"));
		return;
	}
	FPathPickerConfig PickerConfig;
	PickerConfig.DefaultPath = txMeshFolder->GetText().ToString();
	PickerConfig.bNotifyDefaultPathSelected = true;
	PickerConfig.OnPathSelected = FOnPathSelected::CreateWeakLambda(this, [this](const FString& InPath) {
		if (IsValid(txMeshFolder))
		{
			txMeshFolder->SetText(FText::FromString(InPath));
		}
	});

	TSharedRef<SWidget> PathPickerWidget = IContentBrowserSingleton::Get().CreatePathPicker(PickerConfig);

	TSharedRef<SWindow> PickerWindow = SNew(SWindow)
										   .Title(LOCTEXT("SelectMeshFolderWindow", "Select Mesh Folder"))
										   .ClientSize(FVector2D(600, 400))
											   [PathPickerWidget];

	AddModalSubWindow(PickerWindow);
}

void UMeshLODSettingWidget::AddModalSubWindow(TSharedRef<SWindow> SubWindow)
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

void UMeshLODSettingWidget::OnSetLODS()
{
	if (!IsValid(lvLODSettings))
		return;

	// 1. 校验 LOD 设置数据合法性
	//    规则：MaxTriangle 严格递增、LODRedcudes 非空且值 <=1 并按顺序递减
	const auto& Items = lvLODSettings->GetListItems();
	TArray<UMeshLODSettingListItemData*> ValidItems;
	ValidItems.Reserve(Items.Num());
	for (auto Item : Items)
	{
		auto Data = Cast<UMeshLODSettingListItemData>(Item);
		if (IsValid(Data))
			ValidItems.Add(Data);
	}

	FString ErrorMsg;
	if (ValidItems.Num() == 0)
	{
		ErrorMsg = TEXT("LOD 设置列表为空");
	}
	for (int32 i = 0; i < ValidItems.Num() && ErrorMsg.IsEmpty(); ++i)
	{
		auto Item = ValidItems[i];
		if (i > 0 && Item->MaxTriangle <= ValidItems[i - 1]->MaxTriangle)
		{
			ErrorMsg = FString::Printf(TEXT("第 %d 项的 MaxTriangle(%d) 必须大于前一项(%d)"), i + 1, Item->MaxTriangle, ValidItems[i - 1]->MaxTriangle);
		}
		if (Item->LODRedcudes.Num() == 0)
		{
			ErrorMsg = FString::Printf(TEXT("第 %d 项的 LODRedcudes 不能为空"), i + 1);
			continue;
		}
		for (int32 j = 0; j < Item->LODRedcudes.Num(); ++j)
		{
			float Redcude = Item->LODRedcudes[j];
			if (Redcude > 1.0f)
			{
				ErrorMsg = FString::Printf(TEXT("第 %d 项的 LODRedcudes[%d]=%f 大于 1"), i + 1, j, Redcude);
				break;
			}
			if (j > 0 && Redcude >= Item->LODRedcudes[j - 1])
			{
				ErrorMsg = FString::Printf(TEXT("第 %d 项的 LODRedcudes[%d]=%f 未按顺序递减"), i + 1, j, Redcude);
				break;
			}
		}
	}

	if (!ErrorMsg.IsEmpty())
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(ErrorMsg));
		return;
	}

	// 2. 获取目录下所有静态模型（递归）
	FString ModelPath = txMeshFolder->GetText().ToString();
	if (ModelPath.IsEmpty())
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("EmptyMeshFolder", "模型目录为空"));
		return;
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	FARFilter			  Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(FName(*ModelPath));
	Filter.ClassPaths.Add(UStaticMesh::StaticClass()->GetClassPathName());

	TArray<FAssetData> MeshAssets;
	AssetRegistryModule.Get().GetAssets(Filter, MeshAssets);

	// 3. 按三角面数匹配区间，应用对应级别的减面比例
	TArray<int32> RuleCounts;
	RuleCounts.Init(0, ValidItems.Num());
	int32 SetCount = 0;
	for (const FAssetData& Asset : MeshAssets)
	{
		UStaticMesh* Mesh = Cast<UStaticMesh>(Asset.GetAsset());
		if (!IsValid(Mesh))
			continue;

		int32 TriangleCount = Mesh->GetNumTriangles(0); // 用 LOD0 渲染数据的面数决定区间

		// 找到三角面数所在区间的设置项（半开半闭：[PrevMax, Max)），超出最后上限的按最后一项处理
		UMeshLODSettingListItemData* MatchItem = ValidItems.Last();
		for (int32 i = 0; i < ValidItems.Num(); ++i)
		{
			if (TriangleCount < ValidItems[i]->MaxTriangle)
			{
				MatchItem = ValidItems[i];
				break;
			}
		}

		int32 LODCount = MatchItem->LODRedcudes.Num();
		Mesh->SetNumSourceModels(LODCount);
		for (int32 LODIndex = 0; LODIndex < LODCount; ++LODIndex)
		{
			FStaticMeshSourceModel& SrcModelRef = Mesh->GetSourceModel(LODIndex);
			// 屏幕尺寸按 LOD 级数折半递减，避免各级 ScreenSize 相同
			SrcModelRef.ScreenSize = FPerPlatformFloat((LODIndex == 0) ? 1.0f : 1.0f / (float)(1 << LODIndex));
			SrcModelRef.ReductionSettings.PercentTriangles = MatchItem->LODRedcudes[LODIndex];
			if (LODIndex == 0)
			{
				// LOD0 保持原始网格不减面
				SrcModelRef.ReductionSettings.PercentTriangles = 1.0f;
			}
		}

		Mesh->Build(true);
		Mesh->MarkPackageDirty();
		RuleCounts[ValidItems.Find(MatchItem)]++;
		SetCount++;
	}

	// 汇总每条规则命中的模型数
	TArray<FString> RuleLines;
	for (int32 i = 0; i < ValidItems.Num(); ++i)
	{
		RuleLines.Add(FString::Printf(TEXT("<%d面: %d个"), ValidItems[i]->MaxTriangle, RuleCounts[i]));
	}
	FText FinalMsg = FText::Format(NSLOCTEXT("MeshLODSetting", "SetLODSComplete", "已为 {0} 个静态模型设置 LOD\n{1}"), FText::AsNumber(SetCount), FText::FromString(FString::Join(RuleLines, TEXT("\n"))));
	FMessageDialog::Open(EAppMsgType::Ok, FinalMsg);
}

void UMeshLODSettingWidget::OnAddLODSetting()
{
	if (!IsValid(lvLODSettings))
		return;
	int32 MaxTriangle = 0;
	auto& Items = lvLODSettings->GetListItems();
	if (Items.Num() > 0)
	{
		auto LastItem = Cast<UMeshLODSettingListItemData>(Items.Last());
		if (IsValid(LastItem))
			MaxTriangle = LastItem->MaxTriangle;
	}
	MaxTriangle += 500;
	auto Item = NewObject<UMeshLODSettingListItemData>(this);
	Item->MaxTriangle = MaxTriangle;
	Item->LODRedcudes = { 1.0f };
	lvLODSettings->AddItem(Item);
}
void UMeshLODSettingWidget::OnInsertLODSetting(UMeshLODSettingListItemData* BeforeItemData)
{
	if (!IsValid(lvLODSettings) || !IsValid(BeforeItemData))
		return;
	auto  Items = lvLODSettings->GetListItems();
	int32 Index = Items.Find(BeforeItemData);
	if (Index != INDEX_NONE)
	{
		int32 MaxTriangle;
		if (Index > 0)
		{
			auto PrevItem = Cast<UMeshLODSettingListItemData>(Items[Index - 1]);
			if (IsValid(PrevItem))
				MaxTriangle = (BeforeItemData->MaxTriangle + PrevItem->MaxTriangle) / 2;
			else
				MaxTriangle = BeforeItemData->MaxTriangle / 2;
		}
		else
		{
			MaxTriangle = BeforeItemData->MaxTriangle / 2;
		}
		auto Item = NewObject<UMeshLODSettingListItemData>(this);
		Item->MaxTriangle = MaxTriangle;
		Item->LODRedcudes = { 1.0f };
		Items.Insert(Item, Index);
		lvLODSettings->SetListItems(Items);
		lvLODSettings->RegenerateAllEntries();
	}
}
void UMeshLODSettingWidget::OnRemoveLODSetting(UMeshLODSettingListItemData* ItemData)
{
	if (!IsValid(lvLODSettings))
		return;
	lvLODSettings->RemoveItem(ItemData);
}
void UMeshLODSettingWidget::OnItemChanged(UMeshLODSettingListItemData* ItemData)
{
	lvLODSettings->RegenerateAllEntries();
}