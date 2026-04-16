// ModelToIcon.cpp
#include "ModelToIcon.h"
#include "Framework/Application/SlateApplication.h"
// Content Browser
#include "Blueprint/UserWidget.h"

#define LOCTEXT_NAMESPACE "ModelToIcon"


void SModelToIcon::OpenWindow()
{
	if (FSlateApplication::IsInitialized())
	{
		TSharedRef<SModelToIcon> Window = SNew(SModelToIcon);
		FSlateApplication::Get().AddWindow(Window);
	}
}


void SModelToIcon::Construct(const FArguments& InArgs)
{	
	static const FString	 Path = TEXT("/AnimationTools/UI_ModelToIcon.UI_ModelToIcon_C");
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
			.Title(LOCTEXT("ModelToIconTitle", "Model To Icon"))
			.ClientSize(FVector2D(800, 600))
				[MainWidget->TakeWidget()]);
}

#undef LOCTEXT_NAMESPACE
