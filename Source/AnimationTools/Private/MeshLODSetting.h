// ModelToIcon.h
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWindow.h"



class SMeshLODSetting : public SWindow
{
public:
	SLATE_BEGIN_ARGS(SMeshLODSetting) {}
	SLATE_END_ARGS()

	void		Construct(const FArguments& InArgs);
	static void OpenWindow();

private:	
	TStrongObjectPtr<UUserWidget>		 MainWidget;
};
