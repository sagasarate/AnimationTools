#include "AnimationTools.h"
#include "ContentBrowserModule.h"
#include "Animation/AnimSequence.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "AnimationSplitWindow.h"
#include "SlateIconBrowser.h"
#include "AssetRegistry/AssetRegistryModule.h"


#include "FBXBatchImportWin.h"
#include "MeshBrowser.h"
#include "UnityParticleImportWindow.h"
#include "SlateIconBrowser.h"
#include "ModelToIcon.h"
#include "ModelToIconWidget.h"
#include "MeshLODSetting.h"

DEFINE_LOG_CATEGORY(AnimationTools);

#define LOCTEXT_NAMESPACE "AnimationTools"



void FAnimationToolsModule::StartupModule()
{
	// 注册右键菜单
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	{
		TArray<FContentBrowserMenuExtender_SelectedAssets>& MenuExtenders = ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
		MenuExtenders.Add(FContentBrowserMenuExtender_SelectedAssets::CreateRaw(this, &FAnimationToolsModule::OnExtendAssetContextMenu));
	}
	{
		TArray<FContentBrowserMenuExtender_SelectedPaths>& MenuExtenders = ContentBrowserModule.GetAllPathViewContextMenuExtenders();
		MenuExtenders.Add(FContentBrowserMenuExtender_SelectedPaths::CreateRaw(this, &FAnimationToolsModule::OnExtendFolderContentMenu));
	}

	{
		TArray<FContentBrowserMenuExtender_SelectedPaths>& MenuExtenders = ContentBrowserModule.GetAllAssetContextMenuExtenders();
		MenuExtenders.Add(FContentBrowserMenuExtender_SelectedPaths::CreateRaw(this, &FAnimationToolsModule::OnExtendContentMenu));
	}


	// 注册工具栏按钮
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.User");
	if (ToolbarMenu == nullptr)
	{
		ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
	}

	FToolMenuSection& Section = ToolbarMenu->AddSection("BatchFBXImporter", LOCTEXT("BatchFBXImporter", "Batch FBX Importer"));
	Section.AddEntry(FToolMenuEntry::InitToolBarButton(
		"ImportFBXButton",
		FUIAction(FExecuteAction::CreateRaw(this, &FAnimationToolsModule::OnImportFBXClicked)),
		LOCTEXT("ImportFBXLabel", "Batch Import FBX"),
		LOCTEXT("ImportFBXLabel", "Batch Import FBX"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Toolbar.Import")
	));

	Section.AddEntry(FToolMenuEntry::InitToolBarButton(
		"ImportParticlesButton",
		FUIAction(FExecuteAction::CreateRaw(this, &FAnimationToolsModule::OnImportUnityParticlesClicked)),
		LOCTEXT("ImportParticlesLabel", "Import Unity Particles"),
		LOCTEXT("ImportParticlesLabel", "Import Unity Particles"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Convert")
	));

	Section.AddEntry(FToolMenuEntry::InitToolBarButton(
		"BrowseSlateIconButton",
		FUIAction(FExecuteAction::CreateRaw(this, &FAnimationToolsModule::OnBrowseSlateIconClicked)),
		LOCTEXT("BrowseSlateIconLabel", "Browse Slate Icons"),
		LOCTEXT("BrowseSlateIconLabel", "Browse Slate Icons"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Symbols.SearchGlass")
	));

	Section.AddEntry(FToolMenuEntry::InitToolBarButton(
		"ModelToIconButton",
		FUIAction(FExecuteAction::CreateRaw(this, &FAnimationToolsModule::OnModelToIconClicked)),
		LOCTEXT("ModelToIconLabel", "Model To Icon"),
		LOCTEXT("ModelToIconLabel", "Convert Model to Icon"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Settings")
	));

	Section.AddEntry(FToolMenuEntry::InitToolBarButton(
		"MeshLODSettingButton",
		FUIAction(FExecuteAction::CreateRaw(this, &FAnimationToolsModule::OnMeshLODSettingClicked)),
		LOCTEXT("MeshLODSettingLabel", "Mesh LOD Setting"),
		LOCTEXT("MeshLODSettingLabel", "Configure Mesh LOD Settings"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.LOD")
	));

	UToolMenus::Get()->RefreshAllWidgets();
}

void FAnimationToolsModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

TSharedRef<FExtender> FAnimationToolsModule::OnExtendAssetContextMenu(const TArray<FAssetData>& SelectedAssets)
{
	TSharedRef<FExtender> Extender(new FExtender());

	if (SelectedAssets.Num() > 0)
	{
		if(SelectedAssets[0].GetClass() == UAnimSequence::StaticClass())
		{
			Extender->AddMenuExtension(
				"CommonAssetActions",
				EExtensionHook::After,
				nullptr,
				FMenuExtensionDelegate::CreateLambda([this, SelectedAssets](FMenuBuilder& MenuBuilder)
					{
						MenuBuilder.AddMenuEntry(
							LOCTEXT("split_anim", "Split Animation"),
							LOCTEXT("split_anim_desc", "Split the selected animation into segments."),
							FSlateIcon(FAppStyle::Get().GetStyleSetName(), "Icons.Crop"),
							FExecuteAction::CreateLambda([this, SelectedAssets]()
								{
									this->ExecuteSplitAnimation(SelectedAssets);
								})
						);
					})
			);
		}
		else if (SelectedAssets[0].GetClass() == UNiagaraSystem::StaticClass())
		{
			Extender->AddMenuExtension(
				"CommonAssetActions",
				EExtensionHook::After,
				nullptr,
				FMenuExtensionDelegate::CreateLambda([this, SelectedAssets](FMenuBuilder& MenuBuilder)
					{
						MenuBuilder.AddMenuEntry(
							LOCTEXT("DumpNiagaraSystem", "Dump Niagara System"),
							LOCTEXT("DumpNiagaraSystem", "Dump Niagara System"),
							FSlateIcon(FAppStyle::Get().GetStyleSetName(), "Icons.Crop"),
							FExecuteAction::CreateLambda([this, SelectedAssets]()
								{
									for (const FAssetData& AssetData : SelectedAssets)
									{
										if (AssetData.GetClass() == UNiagaraSystem::StaticClass())
										{
											UNiagaraSystem* NiagaraSystem = Cast<UNiagaraSystem>(AssetData.GetAsset());
											if (NiagaraSystem)
												SUnityParticleImportWindow::DumpNiagaraSystem(NiagaraSystem);
										}
									}
								})
						);
						MenuBuilder.AddMenuEntry(
							LOCTEXT("ShowNiagaraSystemGraph", "View Niagara System Graph"),
							LOCTEXT("ShowNiagaraSystemGraph", "View Niagara System Graph"),
							FSlateIcon(FAppStyle::Get().GetStyleSetName(), "Icons.Crop"),
							FExecuteAction::CreateLambda([this, SelectedAssets]()
								{
									for (const FAssetData& AssetData : SelectedAssets)
									{
										if (AssetData.GetClass() == UNiagaraSystem::StaticClass())
										{
											UNiagaraSystem* NiagaraSystem = Cast<UNiagaraSystem>(AssetData.GetAsset());
											if (NiagaraSystem)
												SUnityParticleImportWindow::ShowNiagaraSystemGraph(NiagaraSystem);
										}
									}
								})
						);
					})
			);
		}
	}
	return Extender;
}

TSharedRef<FExtender> FAnimationToolsModule::OnExtendFolderContentMenu(const TArray<FString>& SelectedPaths)
{
	TSharedRef<FExtender> Extender = MakeShared<FExtender>();
	Extender->AddMenuExtension(
		"PathContextBulkOperations",
		EExtensionHook::After,
		nullptr,
		FMenuExtensionDelegate::CreateLambda([this, SelectedPaths](FMenuBuilder& MenuBuilder)
			{
				MenuBuilder.AddMenuEntry(
					LOCTEXT("MenuMeshBrowser", "Browse All Meshs"),
					LOCTEXT("MenuMeshBrowserDesc", "Browse all static and skeletal meshes"),
					FSlateIcon(FAppStyle::Get().GetStyleSetName(), "Icons.NaniteBrowseContent"),
					FExecuteAction::CreateLambda([this, SelectedPaths]()
						{
							this->OnShowMeshBrowser(SelectedPaths);
						})
				);
			})
	);
	return Extender;
}

TSharedRef<FExtender> FAnimationToolsModule::OnExtendContentMenu(const TArray<FString>& SelectedPaths)
{
	TSharedRef<FExtender> Extender = MakeShared<FExtender>();
	Extender->AddMenuExtension(
		"ContentBrowserGetContent",
		EExtensionHook::After,
		nullptr,
		FMenuExtensionDelegate::CreateLambda([this, SelectedPaths](FMenuBuilder& MenuBuilder)
			{
				MenuBuilder.AddMenuEntry(
					LOCTEXT("MenuMeshBrowser", "Browse All Meshs"),
					LOCTEXT("MenuMeshBrowserDesc", "Browse all static and skeletal meshes"),
					FSlateIcon(FAppStyle::Get().GetStyleSetName(), "Icons.NaniteBrowseContent"),
					FExecuteAction::CreateLambda([this, SelectedPaths]()
						{
							this->OnShowMeshBrowser(SelectedPaths);
						})
				);
			})
	);
	return Extender;
}

void FAnimationToolsModule::ExecuteSplitAnimation(const TArray<FAssetData>& SelectedAssets)
{
	UAnimSequence* AnimSequence = Cast<UAnimSequence>(SelectedAssets[0].GetAsset());
	if (!AnimSequence)
		return;


	SAnimationSplitWindow::OpenWindow(AnimSequence);
}

void FAnimationToolsModule::OnShowMeshBrowser(TArray<FString> SelectedPaths)
{
	TArray<FAssetData> MeshAssets;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	for (const FString& Path : SelectedPaths)
	{
		Filter.PackagePaths.Add(FName(*Path));
	}
	Filter.ClassPaths.Add(UStaticMesh::StaticClass()->GetClassPathName());
	Filter.ClassPaths.Add(USkeletalMesh::StaticClass()->GetClassPathName());

	AssetRegistryModule.Get().GetAssets(Filter, MeshAssets);

	SMeshBrowser::OpenWindow(MeshAssets);
}

void FAnimationToolsModule::OnImportFBXClicked()
{
	SFBXBatchImportWin::OpenWindow();
}

void FAnimationToolsModule::OnImportUnityParticlesClicked()
{
	SUnityParticleImportWindow::OpenWindow();
}

void FAnimationToolsModule::OnBrowseSlateIconClicked()
{
	SSlateIconBrowser::OpenWindow();
}

void FAnimationToolsModule::OnModelToIconClicked()
{
	SModelToIcon::OpenWindow();
	// UModelToIconWidget::OpenAsWindow();
}

void FAnimationToolsModule::OnMeshLODSettingClicked()
{
	SMeshLODSetting::OpenWindow();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAnimationToolsModule, AnimationTools)
