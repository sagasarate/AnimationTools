// ModelToIcon.cpp
#include "MeshLODSetting.h"
#include "Framework/Application/SlateApplication.h"
// Content Browser
#include "Blueprint/UserWidget.h"

#define LOCTEXT_NAMESPACE "ModelToIcon"


void SMeshLODSetting::OpenWindow()
{
	if (FSlateApplication::IsInitialized())
	{
		TSharedRef<SMeshLODSetting> Window = SNew(SMeshLODSetting);
		FSlateApplication::Get().AddWindow(Window);
	}
}


void SMeshLODSetting::Construct(const FArguments& InArgs)
{	
	static const FString	 Path = TEXT("/AnimationTools/UI_SetMeshLODS.UI_SetMeshLODS_C");
	TSubclassOf<UUserWidget> WidgetClass = LoadClass<UUserWidget>(nullptr, *Path);
	if (!WidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load widget class for path %s"), *Path);
		return;
	}
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get world context for widget creation"));
		return;
	}
	MainWidget.Reset(CreateWidget<UUserWidget>(World, WidgetClass));
	if (!MainWidget.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create widget instance"));
		return;
	}
	SWindow::Construct(SWindow::FArguments()
			.Title(LOCTEXT("MeshLODSettingTitle", "Mesh LOD Setting"))
			.ClientSize(FVector2D(800, 600))
				[MainWidget->TakeWidget()]);
}

#undef LOCTEXT_NAMESPACE
