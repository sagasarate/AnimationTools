// ModelToIcon.h
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWindow.h"



class SModelToIcon : public SWindow
{
public:
	SLATE_BEGIN_ARGS(SModelToIcon) {}
	SLATE_END_ARGS()

	void		Construct(const FArguments& InArgs);
	static void OpenWindow();

private:	
	TStrongObjectPtr<UUserWidget>		 MainWidget;
};
