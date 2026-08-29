// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/TileView.h"
#include "Components/SpinBox.h"
#include "MeshRenderer.h"
#include "Delegates/IDelegateInstance.h"
#include "Slate/DeferredCleanupSlateBrush.h"
#include "ModelToIconWidget.generated.h"

UCLASS(BlueprintType)
class ANIMATIONTOOLS_API UModelToIconPreviewData : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TWeakObjectPtr<UTexture2D> IconTexture;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString IconName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bExport = true;

	TSharedPtr<FImage> ImageData;
	// 静态/骨骼网格体通用，渲染时按实际类型分流
	TSoftObjectPtr<UObject> SoftMesh;

	UModelToIconPreviewData()
	{
	}
};

UCLASS()
class ANIMATIONTOOLS_API UModelToIconWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()
protected:
	struct RenderTaskInfo
	{
		TSoftObjectPtr<UObject>	 SoftMeshPtr;
		UModelToIconPreviewData* ItemData;
		RenderTaskInfo(const TSoftObjectPtr<UObject>& InSoftMeshPtr, UModelToIconPreviewData* InItemData)
			: SoftMeshPtr(InSoftMeshPtr), ItemData(InItemData)
		{
		}
	};
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> txModelFolder;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> txOutputlFolder;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTileView> tlvPreviewListView;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USpinBox> sbIconSize;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USpinBox> sbMeshRotation;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USpinBox> sbViewRotation;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USpinBox> sbViewAngle;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USpinBox> sbCameraFOV;

	TArray<RenderTaskInfo>		  RenderTaskList;
	int32						  TotalRenderProgress = 0;
	TSharedPtr<SNotificationItem> NotificationItem;
	TUniquePtr<FMeshRenderer>	  MeshRenderer;
	FTSTicker::FDelegateHandle	  TickDelegateHandle;

	int32 CurrentIconSize;
	bool  bTransparentBackground;
	int32 MeshRotationDeg;
	int32 CameraAxisRotationDeg;
	int32 CameraViewAngleDeg;
	float CameraFOVDeg;

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// static void OpenAsWindow();
protected:
	UFUNCTION(BlueprintCallable, Category = "UI")
	void OnSelectInputFolder();
	UFUNCTION(BlueprintCallable, Category = "UI")
	void OnSelectOutputFolder();
	UFUNCTION(BlueprintCallable, Category = "UI")
	void OnGenerateIcons();
	UFUNCTION(BlueprintCallable, Category = "UI")
	void OnReGenerateSelected();
	UFUNCTION(BlueprintCallable, Category = "UI")
	void OnSaveIcons();

	void AddModalSubWindow(TSharedRef<SWindow> SubWindow);
	bool RenderTick(float DeltaTime);
	void FetchRenderParams();
};
