// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/ListView.h"
#include "MeshLODSettingWidget.generated.h"

UCLASS(BlueprintType)
class ANIMATIONTOOLS_API UMeshLODSettingListItemData : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxTriangle = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<float> LODRedcudes;

	UMeshLODSettingListItemData()
	{
	}
};

UCLASS()
class ANIMATIONTOOLS_API UMeshLODSettingWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()
protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> txMeshFolder;
	UPROPERTY(BlueprintReadOnly, Category = "UI", meta = (BindWidget))
	TObjectPtr<UListView> lvLODSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TObjectPtr<UMeshLODSettingListItemData>> LODSettings;

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// static void OpenAsWindow();
protected:
	UFUNCTION(BlueprintCallable, Category = "UI")
	void OnSelectMeshFolder();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void OnSetLODS();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void OnAddLODSetting();
	UFUNCTION(BlueprintCallable, Category = "UI")
	void OnInsertLODSetting(UMeshLODSettingListItemData* BeforeItemData);
	UFUNCTION(BlueprintCallable, Category = "UI")
	void OnRemoveLODSetting(UMeshLODSettingListItemData* ItemData);
	UFUNCTION(BlueprintCallable, Category = "UI")
	void OnItemChanged(UMeshLODSettingListItemData* ItemData);

	void AddModalSubWindow(TSharedRef<SWindow> SubWindow);
};
