#include "ImportProgressWin.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Layout/SBox.h"
#include "Framework/Application/SlateApplication.h"

#define LOCTEXT_NAMESPACE "AnimationTools"

void SImportProgressWin::Construct(const FArguments& InArgs)
{
	SWindow::Construct(SWindow::FArguments()
		.SizingRule(ESizingRule::FixedSize)
		.ClientSize(FVector2D(800, 100))
		.SupportsMinimize(false)
		.SupportsMaximize(false)
		.IsPopupWindow(true)
		.FocusWhenFirstShown(true)
		.IsTopmostWindow(true)  // 让窗口始终置顶
		.SaneWindowPlacement(false) // 允许窗口超出正常范围
	);

	// 构建窗口内容
	SetContent(
		SNew(SBox)
		//.WidthOverride(800.f)
		//.HeightOverride(200.f)
		[
			SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.Padding(10)
				.AutoHeight()
				[
					SAssignNew(StatusTextBlock, STextBlock)
						.Text(LOCTEXT("InitProgress", "Initializing..."))
				]

				+ SVerticalBox::Slot()
				.Padding(10)
				.AutoHeight()
				[
					SAssignNew(ProgressBar, SProgressBar)
						.Percent(0.f)
				]
		]
	);
}

void SImportProgressWin::UpdateProgress(float Progress, const FString& StatusText)
{
	if (ProgressBar.IsValid())
	{
		ProgressBar->SetPercent(Progress);
	}

	if (StatusTextBlock.IsValid())
	{
		StatusTextBlock->SetText(FText::FromString(StatusText));
	}
	FSlateApplication::Get().Tick();
}

void SImportProgressWin::CloseWindow()
{
	FSlateApplication::Get().RequestDestroyWindow(SharedThis(this));
}



#undef LOCTEXT_NAMESPACE