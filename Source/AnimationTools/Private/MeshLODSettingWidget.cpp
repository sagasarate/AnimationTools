// Fill out your copyright notice in the Description page of Project Settings.

#include "MeshLODSettingWidget.h"
#include "Widgets/SWindow.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "EditorUtilitySubsystem.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"

#define LOCTEXT_NAMESPACE "ModelToIcon"

void UMeshLODSettingWidget::NativeConstruct()
{
	Super::NativeConstruct();
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