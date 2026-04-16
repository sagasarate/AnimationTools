#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWindow.h"

class SProgressBar;
class STextBlock;

class SImportProgressWin : public SWindow
{
public:
    SLATE_BEGIN_ARGS(SImportProgressWin) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    // 更新进度条
    void UpdateProgress(float Progress, const FString& StatusText);

    // 关闭窗口
    void CloseWindow();

private:
    TSharedPtr<SProgressBar> ProgressBar;
    TSharedPtr<STextBlock> StatusTextBlock;
};
