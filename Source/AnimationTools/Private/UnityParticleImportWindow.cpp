#include "UnityParticleImportWindow.h"

#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Views/SListView.h"
#include "DesktopPlatformModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "PackageTools.h"
#include "FileHelpers.h"
#include "JsonObjectConverter.h"

#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"
#include "EditorAssetLibrary.h"
#include "AssetToolsModule.h"

#include "NiagaraSystemFactoryNew.h"
#include "NiagaraEmitterFactoryNew.h"
#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraEditorModule.h"
#include "NiagaraEditorUtilities.h"
#include "ViewModels/Stack/NiagaraStackGraphUtilities.h"
#include "NiagaraScriptSource.h"
#include "NiagaraNodeOutput.h"
#include "ViewModels/Stack/NiagaraStackViewModel.h"
#include "ViewModels/Stack/NiagaraStackModuleItem.h"
#include "EdGraphSchema_Niagara.h"
#include "NiagaraDataInterfaceSpriteRendererInfo.h"
#include "NiagaraDataInterfaceColorCurve.h"
#include "NiagaraDataInterfaceCurve.h"

#include "Factories/MaterialFactoryNew.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionParticleColor.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "MaterialEditingLibrary.h"
#include "NiagaraRendererProperties.h"
#include "NiagaraSpriteRendererProperties.h"
#include "ViewModels/NiagaraEmitterViewModel.h"

#include "AssetImportTask.h"
#include "Factories/TextureFactory.h"
#include "EditorFramework/AssetImportData.h"
#include "Engine/Texture.h"
#include "UObject/SavePackage.h"

#define LOCTEXT_NAMESPACE "AnimationTools"

void SUnityParticleImportWindow::Construct(const FArguments &InArgs)
{
	SWindow::Construct(SWindow::FArguments()
			.Title(LOCTEXT("UnityParticleImportWindowTitle", "Unity Particle System Importer"))
			.ClientSize(FVector2D(600, 400))
			.SupportsMaximize(true)
			.SupportsMinimize(true)
				[
					SNew(SVerticalBox) 
						+ SVerticalBox::Slot().AutoHeight().Padding(5)
						[
							SNew(SHorizontalBox) 
								+ SHorizontalBox::Slot().FillWidth(1.0f)
								[
									SAssignNew(m_JsonPathTextBox, SEditableTextBox).HintText(LOCTEXT("JsonPathHint", "JSON file path for import"))
										//.Text(FText::FromString(TEXT("E:/temp/Prefab_effect_huoqiushu02.json")))
								] 
								+ SHorizontalBox::Slot().AutoWidth().Padding(5, 0, 0, 0)
								[
									SNew(SButton).Text(LOCTEXT("BrowseButton", "...")).OnClicked(this, &SUnityParticleImportWindow::OnBrowseJsonPath)
								]
						] 
						+ SVerticalBox::Slot().AutoHeight().Padding(5)
						[
							SNew(SHorizontalBox) 
								+ SHorizontalBox::Slot().FillWidth(1.0f)
								[
									SAssignNew(m_ContentPathTextBox, SEditableTextBox)
										.HintText(LOCTEXT("SavePathHint", "Imported asset save path"))
										.Text(FText::FromString(TEXT("/Game/Models/Effects")))
								] 
								+ SHorizontalBox::Slot().AutoWidth().Padding(5, 0, 0, 0)
								[
									SNew(SButton).Text(LOCTEXT("BrowseButton", "...")).OnClicked(this, &SUnityParticleImportWindow::OnBrowseContentPath)
								]
						] 
						+ SVerticalBox::Slot().FillHeight(1.0f).Padding(5)
						[
							SAssignNew(m_EmitterListView, SListView<TSharedPtr<FUPSParticleSystemData>>)
								//.ItemHeight(24)
								.ListItemsSource(&m_EmitterList)
								.OnGenerateRow(this, &SUnityParticleImportWindow::GenerateEmitterRow)] 
								+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(5)
						 		[
									SNew(SHorizontalBox) 
									+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
									[
										SNew(STextBlock).Text(LOCTEXT("ImportScale", "Scale:"))
									] 
									+ SHorizontalBox::Slot().MinWidth(100.0f).VAlign(VAlign_Center).Padding(2)
									[
										SNew(SEditableTextBox)
											.Text(FText::FromString(FString::SanitizeFloat(m_ImportScale)))
											.OnTextChanged_Lambda([this](const FText &NewText){ LexFromString(m_ImportScale, *NewText.ToString()); })
									] 
									+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2)
									[
										SNew(SCheckBox)
											.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState){ m_AdjustEmitterDuration = (NewState == ECheckBoxState::Checked); })
											.IsChecked_Lambda([this](){ return m_AdjustEmitterDuration ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
											[
												SNew(STextBlock).Text(LOCTEXT("AdjustEmitterDuration", "Adjust Emitter Duration"))
											]
									]
						] 
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(5)
						[
							SNew(SHorizontalBox) 
								+ SHorizontalBox::Slot().AutoWidth().Padding(2)
								[
									SNew(SButton)
										.Text(LOCTEXT("LoadJsonButton", "Load JSON"))
										.OnClicked(this, &SUnityParticleImportWindow::OnLoadJson)
								] 
								+ SHorizontalBox::Slot().AutoWidth().Padding(2)
								[
									SNew(SButton)
									.Text(LOCTEXT("ImportButton", "Import"))
									.OnClicked(this, &SUnityParticleImportWindow::OnImportParticles)
								]
						]
				]
			);
}

FReply SUnityParticleImportWindow::OnBrowseJsonPath()
{
	IDesktopPlatform *DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		TArray<FString> OutFiles;
		if (DesktopPlatform->OpenFileDialog(
				FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
				LOCTEXT("JsonFileDialogTitle", "Select JSON file for import").ToString(),
				FPaths::ProjectDir(),
				TEXT(""),
				TEXT("JSON文件|*.json"),
				EFileDialogFlags::None,
				OutFiles))
		{
			m_JsonPathTextBox->SetText(FText::FromString(OutFiles[0]));
		}
	}
	return FReply::Handled();
}

FReply SUnityParticleImportWindow::OnBrowseContentPath()
{
	FContentBrowserModule &ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	FPathPickerConfig FolderDialogConfig;
	FolderDialogConfig.DefaultPath = m_ContentPathTextBox->GetText().ToString();
	FolderDialogConfig.bAddDefaultPath = true;
	FolderDialogConfig.OnPathSelected.BindRaw(this, &SUnityParticleImportWindow::OnTargetDirSelected);

	TSharedPtr<SWidget> PathPickerWidget = ContentBrowserModule.Get().CreatePathPicker(FolderDialogConfig);

	if (PathPickerWidget.IsValid())
	{
		// 在新窗口中显示路径选择器
		TSharedPtr<SWindow> ParentWindow = SharedThis(this);
		FSlateApplication::Get().AddModalWindow(SNew(SWindow)
													.ClientSize(FVector2D(400, 400))
													.Content()
														[PathPickerWidget.ToSharedRef()],
												ParentWindow);
	}
	return FReply::Handled();
}

void SUnityParticleImportWindow::OnTargetDirSelected(const FString &SelectDir)
{
	m_ContentPathTextBox->SetText(FText::FromString(SelectDir));
}

FReply SUnityParticleImportWindow::OnLoadJson()
{
	FString JsonString;
	if (FFileHelper::LoadFileToString(JsonString, *m_JsonPathTextBox->GetText().ToString()))
	{
		TArray<FUPSParticleSystemData> InfoList;
		if (FJsonObjectConverter::JsonArrayStringToUStruct(JsonString, &InfoList))
		{
			m_EmitterList.Empty();
			for (FUPSParticleSystemData &Info : InfoList)
			{
				m_EmitterList.Add(MakeShared<FUPSParticleSystemData>(Info));
			}
			RefreshEmitterList();
		}
		else
		{
			UE_LOG(AnimationTools, Error, TEXT("Failed to parse JSON file: %s"), *m_JsonPathTextBox->GetText().ToString());
		}
	}
	else
	{
		UE_LOG(AnimationTools, Error, TEXT("Failed to load JSON file: %s"), *m_JsonPathTextBox->GetText().ToString());
	}
	return FReply::Handled();
}

FReply SUnityParticleImportWindow::OnImportParticles()
{
	// TODO: 实现粒子系统导入逻辑
	CreateNiagaraSystem();

	// UNiagaraSystem* NiagaraSystem = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Models/Effects/Prefab_effect_huoqiushu02/Prefab_effect_huoqiushu02.Prefab_effect_huoqiushu02"));
	// CheckNiagaraSystem(NiagaraSystem);
	return FReply::Handled();
}

TSharedRef<ITableRow> SUnityParticleImportWindow::GenerateEmitterRow(TSharedPtr<FUPSParticleSystemData> Item, const TSharedRef<STableViewBase> &OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FUPSParticleSystemData>>, OwnerTable)
		[SNew(SHorizontalBox)
		 // 导入复选框
		 + SHorizontalBox::Slot().AutoWidth()
			   [SNew(SCheckBox)
					.IsChecked_Lambda([Item]()
									  { return Item->beExport ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([Item](ECheckBoxState NewState)
												{ Item->beExport = (NewState == ECheckBoxState::Checked); })]

		 // 文件路径
		 + SHorizontalBox::Slot()
			   .FillWidth(1.0f)
				   [SNew(STextBlock).Text(FText::FromString(Item->name))]];
}

void SUnityParticleImportWindow::RefreshEmitterList()
{
	m_EmitterListView->RequestListRefresh();
}

void SUnityParticleImportWindow::OpenWindow()
{
	TSharedRef<SUnityParticleImportWindow> Window = SNew(SUnityParticleImportWindow);
	FSlateApplication::Get().AddWindow(Window);
}

UNiagaraSystem *SUnityParticleImportWindow::CreateNiagaraSystem()
{
	UClass *FactoryClass = FindObject<UClass>(nullptr, TEXT("/Script/NiagaraEditor.NiagaraSystemFactoryNew"));
	if (!FactoryClass)
	{
		FactoryClass = LoadObject<UClass>(nullptr, TEXT("/Script/NiagaraEditor.NiagaraSystemFactoryNew"));
	}
	if (!FactoryClass)
	{
		UE_LOG(AnimationTools, Error, TEXT("can`t load class:NiagaraSystemFactoryNew"));
		return nullptr;
	}
	UNiagaraSystemFactoryNew *SysytemFactory = (UNiagaraSystemFactoryNew *)(NewObject<UFactory>((UObject *)GetTransientPackage(), FactoryClass));
	if (!SysytemFactory)
	{
		UE_LOG(AnimationTools, Error, TEXT("can`t create UNiagaraSystemFactoryNew"));
		return nullptr;
	}

	// FactoryClass = FindObject<UClass>(nullptr, TEXT("/Script/NiagaraEditor.NiagaraEmitterFactoryNew"));
	// if (!FactoryClass)
	//{
	//	FactoryClass = LoadObject<UClass>(nullptr, TEXT("/Script/NiagaraEditor.NiagaraEmitterFactoryNew"));
	// }
	// if (!FactoryClass)
	//{
	//	UE_LOG(AnimationTools, Error, TEXT("can`t load class:NiagaraEmitterFactoryNew"));
	//	return nullptr;
	// }
	// UNiagaraEmitterFactoryNew* EmitterFactory = (UNiagaraEmitterFactoryNew*)(NewObject<UFactory>((UObject*)GetTransientPackage(), FactoryClass));
	// if (!EmitterFactory)
	//{
	//	UE_LOG(AnimationTools, Error, TEXT("can`t create UNiagaraEmitterFactoryNew"));
	//	return nullptr;
	// }

	FNiagaraEditorModule *NiagaraEditorModule = FModuleManager::GetModulePtr<FNiagaraEditorModule>("NiagaraEditor");
	const UEdGraphSchema_Niagara *NiagaraSchema = GetDefault<UEdGraphSchema_Niagara>();

	FString AssetName = FPaths::GetBaseFilename(m_JsonPathTextBox->GetText().ToString());
	FString ContentDir = MakeUniqueContentDir(m_ContentPathTextBox->GetText().ToString(), AssetName);

	FString FullAssetPath = FPaths::Combine(ContentDir, AssetName);
	if (!FPackageName::IsValidLongPackageName(FullAssetPath))
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ContentPathError", "Invalid imported asset save path!"));
	}

	UPackage *Package = CreatePackage(*FullAssetPath);
	Package->FullyLoad();

	// 2. 创建 Niagara System
	// UE5.8 将 UNiagaraSystemFactoryNew::FactoryCreateNew 的 override 收为 private，经基类指针调用以走公开虚函数分派
	UObject *NewAsset = static_cast<UFactory *>(SysytemFactory)->FactoryCreateNew(UNiagaraSystem::StaticClass(), Package, *AssetName, RF_Standalone | RF_Public, nullptr, GWarn);
	UNiagaraSystem *NiagaraSystem = Cast<UNiagaraSystem>(NewAsset);

	if (!NiagaraSystem)
	{
		UE_LOG(AnimationTools, Error, TEXT("Failed to create Niagara System"));
		return nullptr;
	}

	for (TSharedPtr<FUPSParticleSystemData> UnityParticle : m_EmitterList)
	{
		if (!UnityParticle->beExport)
			continue;

		UNiagaraEmitter *Emitter = NewObject<UNiagaraEmitter>(GetTransientPackage());
		bool bAddDefaultModulesAndRenderers = false;
		UNiagaraEmitterFactoryNew::InitializeEmitter(Emitter, bAddDefaultModulesAndRenderers);
		Emitter->SetUniqueEmitterName(UnityParticle->name);
		Emitter->SetFlags(RF_Transactional);
		Emitter->bIsInheritable = false;

		FGuid EmitterVersion = FGuid::NewGuid();

		FVersionedNiagaraEmitterData *EmitterData = Emitter->GetEmitterData(EmitterVersion);

		UNiagaraScript *EmitterUpdateScript = EmitterData->GetScript(ENiagaraScriptUsage::EmitterUpdateScript, FGuid());

		if (UnityParticle->main.simulationSpace == "World")
			EmitterData->bLocalSpace = false;
		else
			EmitterData->bLocalSpace = true;
		UNiagaraScriptSource *ScriptSource = Cast<UNiagaraScriptSource>(EmitterData->GraphSource);

		// 导入Renderer
		UNiagaraSpriteRendererProperties *SpriteRenderer = nullptr;
		UMaterial *MainMaterial = ImportMaterial(ContentDir, UnityParticle->rendererData.material);
		UMaterial *TailMaterial = ImportMaterial(ContentDir, UnityParticle->rendererData.trailMaterial);
		if (MainMaterial)
		{
			SpriteRenderer = NewObject<UNiagaraSpriteRendererProperties>(Emitter, TEXT("Renderer"));

			// 设置材质
			SpriteRenderer->Material = MainMaterial;

			// （可选）设置一些渲染选项
			SpriteRenderer->SortMode = ENiagaraSortMode::ViewDepth;
			SpriteRenderer->Alignment = ENiagaraSpriteAlignment::Automatic;
			SpriteRenderer->FacingMode = ENiagaraSpriteFacingMode::FaceCamera;

			// 添加到发射器中
			Emitter->AddRenderer(SpriteRenderer, EmitterVersion);
			Emitter->OnPropertiesChanged();

			// 标记发射器为已修改
			Emitter->MarkPackageDirty();

			if (UnityParticle->textureSheetAnimation.enable && UnityParticle->textureSheetAnimation.mode == "Grid")
			{
				SpriteRenderer->SubImageSize.X = UnityParticle->textureSheetAnimation.numTilesX;
				SpriteRenderer->SubImageSize.Y = UnityParticle->textureSheetAnimation.numTilesY;
			}
		}
		UNiagaraNodeFunctionCall *FuncNode = nullptr;
		// 导入主模块

		// Simulation Space
		EmitterData->bLocalSpace = UnityParticle->main.simulationSpace == TEXT("Local");
		// 发射器状态模块
		FuncNode = AddEmitterModule(EmitterData, ENiagaraScriptUsage::EmitterUpdateScript, TEXT("/Niagara/Modules/Emitter/EmitterState.EmitterState"));
		if (FuncNode)
		{
			// 设置静态开关
			SetEmitterModulePinWithEnum(FuncNode, TEXT("Life Cycle Mode"), TEXT("/Niagara/Enums/ENiagaraEmitterLifeCycleMode.ENiagaraEmitterLifeCycleMode"), 1);
			// Looping
			SetEmitterModulePinWithEnum(FuncNode, TEXT("Loop Behavior"), TEXT("/Niagara/Enums/ENiagara_EmitterStateOptions.ENiagara_EmitterStateOptions"), UnityParticle->main.loop ? 0 : 1);
			// Start Delay
			if (HaveValue(UnityParticle->main.startDelay))
			{
				SetEmitterModulePin(FuncNode, TEXT("UseLoopDelay"), LexToSanitizedString(true));
				SetupModuleFloatCurveParam(EmitterData, FuncNode, TEXT("Loop Delay"), UnityParticle->main.startDelay);
			}
			FuncNode->MarkPackageDirty();
		}
		// 粒子状态模块
		FuncNode = AddEmitterModule(EmitterData, ENiagaraScriptUsage::ParticleUpdateScript, TEXT("/Niagara/Modules/Update/Lifetime/ParticleState.ParticleState"));
		if (FuncNode)
		{
			FuncNode->MarkPackageDirty();
		}
		// 粒子初始化模块
		FuncNode = AddEmitterModule(EmitterData, ENiagaraScriptUsage::ParticleSpawnScript, TEXT("/Niagara/Modules/Spawn/Initialization/V2/InitializeParticle.InitializeParticle"));
		if (FuncNode)
		{
			// Start Lifetime
			UnityParticle->main.startLifetime.BindModuleName = FuncNode->GetFunctionName();
			SetEmitterModulePinWithEnum(FuncNode, TEXT("Lifetime Mode"), TEXT("/Niagara/Enums/ENiagara_LifetimeMode.ENiagara_LifetimeMode"), UnityParticle->main.startLifetime.mode == TEXT("TwoConstants") ? 1 : 0);
			// Start Color 不支持Gradient模式,该模式会选择不设置出生颜色
			UnityParticle->main.startColor.BindModuleName = FuncNode->GetFunctionName();
			if (UnityParticle->main.startColor.mode == TEXT("TwoColors"))
				SetEmitterModulePinWithEnum(FuncNode, TEXT("Color Mode"), TEXT("/Niagara/Enums/ENiagara_ColorInitializationMode.ENiagara_ColorInitializationMode"), 2);
			else if (UnityParticle->main.startColor.mode == TEXT("Color"))
				SetEmitterModulePinWithEnum(FuncNode, TEXT("Color Mode"), TEXT("/Niagara/Enums/ENiagara_ColorInitializationMode.ENiagara_ColorInitializationMode"), 1);
			else
				SetEmitterModulePinWithEnum(FuncNode, TEXT("Color Mode"), TEXT("/Niagara/Enums/ENiagara_ColorInitializationMode.ENiagara_ColorInitializationMode"), 0);
			SetEmitterModulePin(FuncNode, TEXT("UsePositionOffset"), LexToSanitizedString(true));
			// Start Size 不支持Curve
			UnityParticle->main.startSize.BindModuleName = FuncNode->GetFunctionName();
			if (HaveValue(UnityParticle->main.startSize))
			{
				if (UnityParticle->main.startSize.mode == TEXT("TwoConstants"))
					SetEmitterModulePinWithEnum(FuncNode, TEXT("Sprite Size Mode"), TEXT("/Niagara/Enums/ENiagara_SizeScaleMode.ENiagara_SizeScaleMode"), 4);
				else if (UnityParticle->main.startSize.mode == TEXT("Constant"))
					SetEmitterModulePinWithEnum(FuncNode, TEXT("Sprite Size Mode"), TEXT("/Niagara/Enums/ENiagara_SizeScaleMode.ENiagara_SizeScaleMode"), 3);
				else
					SetEmitterModulePinWithEnum(FuncNode, TEXT("Sprite Size Mode"), TEXT("/Niagara/Enums/ENiagara_SizeScaleMode.ENiagara_SizeScaleMode"), 0);
			}
			else
			{
				SetEmitterModulePinWithEnum(FuncNode, TEXT("Sprite Size Mode"), TEXT("/Niagara/Enums/ENiagara_SizeScaleMode.ENiagara_SizeScaleMode"), 0);
			}
			// Start Rotation
			UnityParticle->main.startRotation.BindModuleName = FuncNode->GetFunctionName();
			if (HaveValue(UnityParticle->main.startRotation))
			{
				SetEmitterModulePinWithEnum(FuncNode, TEXT("Sprite Rotation Mode"), TEXT("/Niagara/Enums/ENiagara_SpriteRotationMode.ENiagara_SpriteRotationMode"), 2);
				SetupModuleFloatCurveParam(EmitterData, FuncNode, TEXT("Sprite Rotation Angle"), UnityParticle->main.startRotation);
			}
			else
			{
				SetEmitterModulePinWithEnum(FuncNode, TEXT("Sprite Rotation Mode"), TEXT("/Niagara/Enums/ENiagara_SpriteRotationMode.ENiagara_SpriteRotationMode"), 0);
			}
			FuncNode->MarkPackageDirty();
		}
		// 导入shape模块
		if (UnityParticle->shape.enable)
		{
			FuncNode = AddEmitterModule(EmitterData, ENiagaraScriptUsage::ParticleSpawnScript, TEXT("/Niagara/Modules/Spawn/Location/V2/ShapeLocation.ShapeLocation"));
			if (FuncNode)
			{
				UnityParticle->shape.arcSpeed.BindModuleName = FuncNode->GetFunctionName();
				if (UnityParticle->shape.shapeType == TEXT("Sphere"))
				{
					// 球形
					SetEmitterModulePinWithEnum(FuncNode, TEXT("Shape Primitive"), TEXT("/Niagara/Enums/Location/ENiagara_LocationShapes.ENiagara_LocationShapes"), 0);
				}
				else if (UnityParticle->shape.shapeType == TEXT("Box"))
				{
					SetEmitterModulePinWithEnum(FuncNode, TEXT("Shape Primitive"), TEXT("/Niagara/Enums/Location/ENiagara_LocationShapes.ENiagara_LocationShapes"), 2);
					SetEmitterModulePinWithEnum(FuncNode, TEXT("Box / Plane Mode"), TEXT("/Niagara/Enums/Location/ENiagara_BoxPlaneMode.ENiagara_BoxPlaneMode"), 0);
				}
				else if (UnityParticle->shape.shapeType == TEXT("Cone"))
				{
					// 圆锥型
					SetEmitterModulePinWithEnum(FuncNode, TEXT("Shape Primitive"), TEXT("/Niagara/Enums/Location/ENiagara_LocationShapes.ENiagara_LocationShapes"), 5);
					SetEmitterModulePinWithEnum(FuncNode, TEXT("Cone Mode Type"), TEXT("/Niagara/Enums/Location/ENiagara_ConeMode.ENiagara_ConeMode"), 0);
				}
				SetEmitterModulePinWithEnum(FuncNode, TEXT("Transform Order"), TEXT("/Niagara/Enums/Transforms/ENiagara_TransformOrder.ENiagara_TransformOrder"), 0);
				SetEmitterModulePinWithEnum(FuncNode, TEXT("Scale Mode"), TEXT("/Niagara/Enums/Transforms/ENiagara_ScaleMode.ENiagara_ScaleMode"), 0);
				SetEmitterModulePinWithEnum(FuncNode, TEXT("Rotation Mode"), TEXT("/Niagara/Enums/Transforms/ENiagara_RotationMode.ENiagara_RotationMode"), 2);
				SetEmitterModulePinWithEnum(FuncNode, TEXT("Rotation Angle Type"), TEXT("/Niagara/Enums/Angles/ENiagara_AngleInput.ENiagara_AngleInput"), 0);
				SetEmitterModulePinWithEnum(FuncNode, TEXT("Offset Mode"), TEXT("/Niagara/Enums/Transforms/ENiagara_OffsetMode.ENiagara_OffsetMode"), 0);
			}
		}
		// 速度模块
		if (HaveValue(UnityParticle->main.startSpeed))
		{
			FuncNode = AddEmitterModule(EmitterData, ENiagaraScriptUsage::ParticleSpawnScript, TEXT("/Niagara/Modules/Spawn/Velocity/AddVelocity.AddVelocity"));
			if (FuncNode)
			{
				// Start Speed
				UnityParticle->main.startSpeed.BindModuleName = FuncNode->GetFunctionName();
				if (UnityParticle->shape.enable && UnityParticle->shape.shapeType == TEXT("Sphere"))
				{
					// sphere模式是随机像四周发射的，所以采用标量形式的速度
					SetEmitterModulePinWithEnum(FuncNode, TEXT("Velocity Mode"), TEXT("/Niagara/Enums/Utility/ENiagara_VelocityMode.ENiagara_VelocityMode"), 1);
					SetupModuleFloatCurveParam(EmitterData, FuncNode, TEXT("Velocity Speed"), UnityParticle->main.startSpeed);
				}
				else if (UnityParticle->shape.enable && UnityParticle->shape.shapeType == TEXT("Cone"))
				{
					// 速度也配合使用Cone模式,采用标量形式的速度
					SetEmitterModulePinWithEnum(FuncNode, TEXT("Velocity Mode"), TEXT("/Niagara/Enums/Utility/ENiagara_VelocityMode.ENiagara_VelocityMode"), 2);
					SetupModuleFloatCurveParam(EmitterData, FuncNode, TEXT("Velocity Speed"), UnityParticle->main.startSpeed);
				}
				else
				{
					// 使用向量形式的速度，以可以应用旋转
					SetEmitterModulePinWithEnum(FuncNode, TEXT("Velocity Mode"), TEXT("/Niagara/Enums/Utility/ENiagara_VelocityMode.ENiagara_VelocityMode"), 0);
					SetupModuleSpeedCurveParam(EmitterData, FuncNode, TEXT("Velocity"), UnityParticle->main.startSpeed);
				}

				FuncNode->MarkPackageDirty();
			}
		}

		// 导入Emission模块
		if (UnityParticle->emission.enable)
		{
			if (HaveValue(UnityParticle->emission.rateOverTime))
			{
				// 粒子生成率模块
				FuncNode = AddEmitterModule(EmitterData, ENiagaraScriptUsage::EmitterUpdateScript, TEXT("/Niagara/Modules/Emitter/SpawnRate.SpawnRate"));
				if (FuncNode)
				{
					// rateOverTime
					UnityParticle->emission.rateOverTime.BindModuleName = FuncNode->GetFunctionName();
					SetupModuleFloatCurveParam(EmitterData, FuncNode, TEXT("SpawnRate"), UnityParticle->emission.rateOverTime);
					FuncNode->MarkPackageDirty();
				}
			}
			if (UnityParticle->emission.bursts.Num())
			{
				for (auto &burst : UnityParticle->emission.bursts)
				{
					int ModuleCount = burst.cycleCount;
					if (ModuleCount <= 0)
						ModuleCount = 1;
					for (int i = 0; i < ModuleCount; i++)
					{
						FuncNode = AddEmitterModule(EmitterData, ENiagaraScriptUsage::EmitterUpdateScript, TEXT("/Niagara/Modules/Emitter/SpawnBurst_Instantaneous.SpawnBurst_Instantaneous"));
						if (FuncNode)
						{
							burst.BindModuleNames.Add(FuncNode->GetFunctionName());
							SetupModuleFloatCurveParam(EmitterData, FuncNode, TEXT("Spawn Count"), burst.count);
							SetEmitterModulePin(FuncNode, TEXT("Use Spawn Probability"), LexToSanitizedString(true));
							FuncNode->MarkPackageDirty();
						}
						else
						{
							burst.BindModuleNames.Add(TEXT("Error"));
						}
					}
				}
			}
		}
		else
		{
			FuncNode = AddEmitterModule(EmitterData, ENiagaraScriptUsage::EmitterUpdateScript, TEXT("/Niagara/Modules/Emitter/SpawnBurst_Instantaneous.SpawnBurst_Instantaneous"));
			if (FuncNode)
			{
				UnityParticle->emission.rateOverTime.BindModuleName = FuncNode->GetFunctionName();
				FuncNode->MarkPackageDirty();
			}
		}

		// 导入ColorOverLifetime模块
		if (UnityParticle->colorOverLifetime.enable)
		{
			FuncNode = AddEmitterModule(EmitterData, ENiagaraScriptUsage::ParticleUpdateScript, TEXT("/Niagara/Modules/Update/Color/ScaleColor.ScaleColor"));
			if (FuncNode)
			{
				UnityParticle->colorOverLifetime.color.BindModuleName = FuncNode->GetFunctionName();
				SetupScaleColorModule(EmitterData, FuncNode, UnityParticle->colorOverLifetime.color);
				FuncNode->MarkPackageDirty();
			}
		}

		// 导入SizeOverLifetime模块
		if (UnityParticle->sizeOverLifetime.enable)
		{
			FuncNode = AddEmitterModule(EmitterData, ENiagaraScriptUsage::ParticleUpdateScript, TEXT("/Niagara/Modules/Update/Size/ScaleSpriteSize.ScaleSpriteSize"));
			if (FuncNode)
			{
				UnityParticle->sizeOverLifetime.size.BindModuleName = FuncNode->GetFunctionName();
				SetupScaleSpriteSizeModule(EmitterData, FuncNode, UnityParticle->sizeOverLifetime.size);
				FuncNode->MarkPackageDirty();
			}
		}

		// 帧动画模块
		if (UnityParticle->textureSheetAnimation.enable && UnityParticle->textureSheetAnimation.mode == "Grid")
		{
			FuncNode = AddSubUVAnimationModule(EmitterData, SpriteRenderer);
			if (FuncNode)
			{
				FuncNode->MarkPackageDirty();
			}
		}

		if (HaveValue(UnityParticle->main.startSpeed))
		{
			// 力学初始化应用模块
			FuncNode = AddEmitterModule(EmitterData, ENiagaraScriptUsage::ParticleSpawnScript, TEXT("/Niagara/Modules/Solvers/ApplyInitialForces.ApplyInitialForces"));
			if (FuncNode)
			{
				FuncNode->MarkPackageDirty();
			}

			// 力学更新模块
			FuncNode = AddEmitterModule(EmitterData, ENiagaraScriptUsage::ParticleUpdateScript, TEXT("/Niagara/Modules/Solvers/SolveForcesAndVelocity.SolveForcesAndVelocity"));
			if (FuncNode)
			{
				FuncNode->MarkPackageDirty();
			}
		}

		if (UnityParticle->rendererData.enabled)
		{
			if (UnityParticle->rendererData.renderMode == TEXT("Stretch"))
			{
				SpriteRenderer->Alignment = ENiagaraSpriteAlignment::VelocityAligned;
				FuncNode = AddEmitterModule(EmitterData, ENiagaraScriptUsage::ParticleUpdateScript, TEXT("/Niagara/Modules/Update/Size/ScaleSpriteSizeBySpeed.ScaleSpriteSizeBySpeed"));
				if (FuncNode)
				{
					UnityParticle->rendererData.ScaleSpriteSizeBySpeedModuleName = FuncNode->GetFunctionName();
					FuncNode->MarkPackageDirty();
				}
			}
			else if (UnityParticle->rendererData.renderMode == TEXT("HorizontalBillboard"))
			{
				SpriteRenderer->FacingMode = ENiagaraSpriteFacingMode::CustomFacingVector;
				FuncNode = AddEmitterModule(EmitterData, ENiagaraScriptUsage::ParticleSpawnScript, TEXT("/Niagara/Modules/Update/Renderers/Sprite/SpriteFacingAndAlignment.SpriteFacingAndAlignment"));
				if (FuncNode)
				{
					UnityParticle->rendererData.SpriteFacingModuleName = FuncNode->GetFunctionName();
					SetEmitterModulePin(FuncNode, TEXT("WriteAlignment"), LexToSanitizedString(false));
					SetEmitterModulePin(FuncNode, TEXT("WriteFacing"), LexToSanitizedString(true));
					FuncNode->MarkPackageDirty();
				}
			}
		}

		UnityParticle->EmitterHandleID = FNiagaraEditorUtilities::AddEmitterToSystem(*NiagaraSystem, *Emitter, EmitterVersion, false);

		ScriptSource->NodeGraph->NotifyGraphChanged();
		ScriptSource->NodeGraph->MarkPackageDirty();
		ScriptSource->MarkPackageDirty();
	}

	// 编译系统
	NiagaraSystem->RequestCompile(true);
	NiagaraSystem->WaitForCompilationComplete();

	// NiagaraSystem->RequestCompile(false);
	// NiagaraSystem->WaitForCompilationComplete();

	// DumpNiagaraSystem(NiagaraSystem);

	for (TSharedPtr<FUPSParticleSystemData> UnityParticle : m_EmitterList)
	{
		const FVersionedNiagaraEmitter *VersionedEmitter = nullptr;
		for (int i = 0; i < NiagaraSystem->GetNumEmitters(); i++)
		{
			FNiagaraEmitterHandle &Handle = NiagaraSystem->GetEmitterHandle(i);
			if (Handle.GetId() == UnityParticle->EmitterHandleID)
			{
				const FVersionedNiagaraEmitter& VersionedEmitterRef = Handle.GetInstance();
				VersionedEmitter = &VersionedEmitterRef;
				break;
			}
		}
		if (VersionedEmitter)
		{
			// 参数设置
			FVersionedNiagaraEmitterData *EmitterData = const_cast<FVersionedNiagaraEmitterData*>(VersionedEmitter->GetEmitterData());
			UNiagaraScriptSource *ScriptSource = Cast<UNiagaraScriptSource>(EmitterData->GraphSource);
			ScriptSource->NodeGraph->Modify();

			UNiagaraScript *EmitterUpdateScript = EmitterData->GetScript(ENiagaraScriptUsage::EmitterUpdateScript, FGuid());
			UNiagaraScript *ParticleSpawnScript = EmitterData->GetScript(ENiagaraScriptUsage::ParticleSpawnScript, FGuid());
			UNiagaraScript *ParticleUpdateScript = EmitterData->GetScript(ENiagaraScriptUsage::ParticleUpdateScript, FGuid());

			FVector3f EmitterOffset = UnityPosition2UE(UnityParticle->position) * m_ImportScale;
			FVector3f EmitterRotation = UnityRotation2UE(UnityParticle->rotation);
			FQuat4f EmitterRotator = FQuat4f::MakeFromEuler(EmitterRotation);
			FVector3f EmitterScale = UnityPosition2UE(UnityParticle->scale);
			FTransform3f EmitterTransform(EmitterRotator, EmitterOffset, EmitterScale);

			// Duration
			if (!UnityParticle->main.loop)
			{
				float Duration = UnityParticle->main.duration;
				if (m_AdjustEmitterDuration)
				{
					if (!HaveValue(UnityParticle->emission.rateOverTime))
					{
						if (UnityParticle->emission.bursts.Num())
						{
							Duration = 0;
							for (auto &burst : UnityParticle->emission.bursts)
							{
								int ModuleCount = burst.cycleCount;
								if (ModuleCount <= 0)
									ModuleCount = 1;
								float CurDuration = (burst.time + UnityParticle->main.startLifetime.constantMax + burst.repeatInterval) * ModuleCount;
								if (CurDuration > Duration)
									Duration = CurDuration;
							}
						}
						else
						{
							Duration = UnityParticle->main.startLifetime.constantMax;
						}
					}
				}
				SetScriptParamValue(EmitterUpdateScript, *FString::Printf(TEXT("Constants.%s.EmitterState.Loop Duration"), *VersionedEmitter->Emitter->GetUniqueEmitterName()), Duration);
			}
			// Start Delay
			if (HaveValue(UnityParticle->main.startDelay))
			{
				SetScriptFloatParamUnityCurve(EmitterUpdateScript, *VersionedEmitter->Emitter->GetUniqueEmitterName(), TEXT("Loop Delay"), UnityParticle->main.startDelay);
			}
			// Start Lifetime
			if (UnityParticle->main.startLifetime.mode == TEXT("TwoConstants"))
			{
				SetScriptParamValue(ParticleSpawnScript,
									*FString::Printf(TEXT("Constants.%s.%s.Lifetime Min"), *VersionedEmitter->Emitter->GetUniqueEmitterName(), *UnityParticle->main.startLifetime.BindModuleName),
									UnityParticle->main.startLifetime.constantMin);
				SetScriptParamValue(ParticleSpawnScript,
									*FString::Printf(TEXT("Constants.%s.%s.Lifetime Max"), *VersionedEmitter->Emitter->GetUniqueEmitterName(), *UnityParticle->main.startLifetime.BindModuleName),
									UnityParticle->main.startLifetime.constantMax);
			}
			else
			{
				SetScriptParamValue(ParticleSpawnScript,
									*FString::Printf(TEXT("Constants.%s.%s.Lifetime"), *VersionedEmitter->Emitter->GetUniqueEmitterName(), *UnityParticle->main.startLifetime.BindModuleName),
									UnityParticle->main.startLifetime.constantMax);
			}
			// Start Color 不支持Gradient模式
			if (UnityParticle->main.startColor.mode == TEXT("TwoColors"))
			{
				SetScriptParamValue(ParticleSpawnScript,
									*FString::Printf(TEXT("Constants.%s.%s.Color Minimum"), *VersionedEmitter->Emitter->GetUniqueEmitterName(), *UnityParticle->main.startColor.BindModuleName),
									ColorStr2LinearColor(UnityParticle->main.startColor.colorMin));
				SetScriptParamValue(ParticleSpawnScript,
									*FString::Printf(TEXT("Constants.%s.%s.Color Maximum"), *VersionedEmitter->Emitter->GetUniqueEmitterName(), *UnityParticle->main.startColor.BindModuleName),
									ColorStr2LinearColor(UnityParticle->main.startColor.colorMax));
			}
			else if (UnityParticle->main.startColor.mode == TEXT("Color"))
			{
				SetScriptParamValue(ParticleSpawnScript,
									*FString::Printf(TEXT("Constants.%s.%s.Color"), *VersionedEmitter->Emitter->GetUniqueEmitterName(), *UnityParticle->main.startColor.BindModuleName),
									ColorStr2LinearColor(UnityParticle->main.startColor.colorMax));
			}
			SetScriptParamValue(ParticleSpawnScript,
								*FString::Printf(TEXT("Constants.%s.%s.Position Offset"), *VersionedEmitter->Emitter->GetUniqueEmitterName(), *UnityParticle->main.startLifetime.BindModuleName),
								EmitterOffset);
			// Start Size 不支持Curve
			if (HaveValue(UnityParticle->main.startSize))
			{
				FVector2f SizeMin(UnityParticle->main.startSize.constantMin * EmitterScale.X * m_ImportScale, UnityParticle->main.startSize.constantMin * EmitterScale.Y * m_ImportScale);
				FVector2f SizeMax(UnityParticle->main.startSize.constantMax * EmitterScale.X * m_ImportScale, UnityParticle->main.startSize.constantMax * EmitterScale.Y * m_ImportScale);
				if (UnityParticle->main.startSize.mode == TEXT("TwoConstants"))
				{
					SetScriptParamValue(ParticleSpawnScript,
										*FString::Printf(TEXT("Constants.%s.%s.Sprite Size Min"), *VersionedEmitter->Emitter->GetUniqueEmitterName(), *UnityParticle->main.startSize.BindModuleName),
										SizeMin);
					SetScriptParamValue(ParticleSpawnScript,
										*FString::Printf(TEXT("Constants.%s.%s.Sprite Size Max"), *VersionedEmitter->Emitter->GetUniqueEmitterName(), *UnityParticle->main.startSize.BindModuleName),
										SizeMax);
				}
				else if (UnityParticle->main.startSize.mode == TEXT("Constant"))
				{
					SetScriptParamValue(ParticleSpawnScript,
										*FString::Printf(TEXT("Constants.%s.%s.Sprite Size"), *VersionedEmitter->Emitter->GetUniqueEmitterName(), *UnityParticle->main.startSize.BindModuleName),
										SizeMax);
				}
			}
			// Start Rotation
			if (HaveValue(UnityParticle->main.startRotation))
			{
				FUPSSerializableCurve Rotation = UnityParticle->main.startRotation;
				Rotation.RadiansToDegrees();
				Rotation.Rotate(EmitterRotator.ToRotationVector().X);
				SetScriptFloatParamUnityCurve(ParticleSpawnScript, *VersionedEmitter->Emitter->GetUniqueEmitterName(), TEXT("Sprite Rotation Angle"), Rotation);
			}

			if (UnityParticle->emission.enable)
			{
				if (HaveValue(UnityParticle->emission.rateOverTime))
				{
					// rateOverTime
					SetScriptFloatParamUnityCurve(EmitterUpdateScript, *VersionedEmitter->Emitter->GetUniqueEmitterName(), TEXT("SpawnRate"), UnityParticle->emission.rateOverTime);
				}
				if (UnityParticle->emission.bursts.Num())
				{
					for (auto &burst : UnityParticle->emission.bursts)
					{
						int ModuleCount = burst.cycleCount;
						if (ModuleCount <= 0)
							ModuleCount = 1;
						for (int i = 0; i < ModuleCount; i++)
						{
							SetScriptParamValue(EmitterUpdateScript,
												*FString::Printf(TEXT("Constants.%s.%s.Spawn Time"), *VersionedEmitter->Emitter->GetUniqueEmitterName(), *burst.BindModuleNames[i]),
												burst.time + i * burst.repeatInterval);
							SetScriptIntParamUnityCurve(EmitterUpdateScript, *VersionedEmitter->Emitter->GetUniqueEmitterName(), TEXT("Spawn Count"), burst.count, 1.0f, *burst.BindModuleNames[i]);
							SetScriptParamValue(EmitterUpdateScript,
												*FString::Printf(TEXT("Constants.%s.%s.Spawn Probability"), *VersionedEmitter->Emitter->GetUniqueEmitterName(), *burst.BindModuleNames[i]),
												burst.probability);
						}
					}
				}
			}
			else
			{
				SetScriptParamValue(EmitterUpdateScript,
									*FString::Printf(TEXT("Constants.%s.%s.Spawn Count"), *VersionedEmitter->Emitter->GetUniqueEmitterName(), *UnityParticle->emission.rateOverTime.BindModuleName),
									(int32)1);
			}

			// maxParticles
			if (UnityParticle->main.maxParticles > 0)
			{
				EmitterData->AllocationMode = EParticleAllocationMode::FixedCount;
				EmitterData->PreAllocationCount = UnityParticle->main.maxParticles;
			}

			// shape模块
			if (UnityParticle->shape.enable)
			{
				if (UnityParticle->shape.shapeType == TEXT("Sphere"))
				{
					// 球形
					SetScriptParamValue(ParticleSpawnScript,
										*FString::Printf(TEXT("Constants.%s.%s.Sphere Radius"), *VersionedEmitter->Emitter->GetUniqueEmitterName(), *UnityParticle->shape.arcSpeed.BindModuleName),
										UnityParticle->shape.radius);
				}
				else if (UnityParticle->shape.shapeType == TEXT("Box"))
				{
					SetScriptParamValue(ParticleSpawnScript,
										*FString::Printf(TEXT("Constants.%s.%s.Box Size"), *VersionedEmitter->Emitter->GetUniqueEmitterName(), *UnityParticle->shape.arcSpeed.BindModuleName),
										FVector3f(1, 1, 1));
				}
				else if (UnityParticle->shape.shapeType == TEXT("Cone"))
				{
					// 圆锥型
					float angle = FMath::RadiansToDegrees(UnityParticle->shape.angle);
					SetScriptParamValue(ParticleSpawnScript,
										*FString::Printf(TEXT("Constants.%s.%s.Cone Angle"), *VersionedEmitter->Emitter->GetUniqueEmitterName(), *UnityParticle->shape.arcSpeed.BindModuleName),
										angle);
					SetScriptParamValue(ParticleSpawnScript,
										*FString::Printf(TEXT("Constants.%s.%s.Cone Length"), *VersionedEmitter->Emitter->GetUniqueEmitterName(), *UnityParticle->shape.arcSpeed.BindModuleName),
										UnityParticle->shape.radius * m_ImportScale);
					// SetScriptParamValue(ParticleSpawnScript,
					//	*FString::Printf(TEXT("Constants.%s.%s.Rotation Axis"), *VersionedEmitter->Emitter->GetUniqueEmitterName(), *UnityParticle->main.startSpeed.BindModuleName),
					//	FVector(1, 0, 0));
				}
				SetScriptParamValue(ParticleSpawnScript,
									*FString::Printf(TEXT("Constants.%s.%s.Non Uniform Scale"), *VersionedEmitter->Emitter->GetUniqueEmitterName(), *UnityParticle->shape.arcSpeed.BindModuleName),
									UnityPosition2UE(UnityParticle->shape.scale) * EmitterScale * m_ImportScale);
				FQuat4f Rotation = FQuat4f::MakeFromEuler(UnityRotation2UE(UnityParticle->shape.rotation)) * EmitterRotator;
				FVector3f RotationVec = FMath::RadiansToDegrees(Rotation.ToRotationVector());
				SetScriptParamValue(ParticleSpawnScript,
									*FString::Printf(TEXT("Constants.%s.%s.Yaw / Pitch / Roll"), *VersionedEmitter->Emitter->GetUniqueEmitterName(), *UnityParticle->shape.arcSpeed.BindModuleName),
									FVector3f(RotationVec.Z, RotationVec.Y, RotationVec.X));
				SetScriptParamValue(ParticleSpawnScript,
									*FString::Printf(TEXT("Constants.%s.%s.Offset"), *VersionedEmitter->Emitter->GetUniqueEmitterName(), *UnityParticle->shape.arcSpeed.BindModuleName),
									UnityPosition2UE(UnityParticle->shape.position) * m_ImportScale + EmitterOffset);
			}
			// Start Speed
			if (HaveValue(UnityParticle->main.startSpeed))
			{
				if (UnityParticle->shape.enable && UnityParticle->shape.shapeType == TEXT("Sphere"))
				{
					SetScriptFloatParamUnityCurve(ParticleSpawnScript, *VersionedEmitter->Emitter->GetUniqueEmitterName(), TEXT("Velocity"), UnityParticle->main.startSpeed, m_ImportScale);
					SetScriptParamValue(ParticleSpawnScript,
										*FString::Printf(TEXT("Constants.%s.%s.Origin Offset"), *VersionedEmitter->Emitter->GetUniqueEmitterName(), *UnityParticle->main.startSpeed.BindModuleName),
										FVector3f::ZeroVector);
				}
				else if (UnityParticle->shape.enable && UnityParticle->shape.shapeType == TEXT("Cone"))
				{
					SetScriptFloatParamUnityCurve(ParticleSpawnScript, *VersionedEmitter->Emitter->GetUniqueEmitterName(), TEXT("Velocity"), UnityParticle->main.startSpeed, m_ImportScale);
					SetScriptParamValue(ParticleSpawnScript,
										*FString::Printf(TEXT("Constants.%s.%s.Cone Axis"), *VersionedEmitter->Emitter->GetUniqueEmitterName(), *UnityParticle->main.startSpeed.BindModuleName),
										FVector3f(1, 0, 0));
					float angle = FMath::RadiansToDegrees(UnityParticle->shape.angle);
					SetScriptParamValue(ParticleSpawnScript,
										*FString::Printf(TEXT("Constants.%s.%s.Cone Angle"), *VersionedEmitter->Emitter->GetUniqueEmitterName(), *UnityParticle->main.startSpeed.BindModuleName),
										angle);
				}
				else
				{
					SetScriptSpeedParamUnityCurve(ParticleSpawnScript, *VersionedEmitter->Emitter->GetUniqueEmitterName(), TEXT("Velocity"), UnityParticle->main.startSpeed, m_ImportScale, EmitterRotator);
				}
			}
			// Color模块
			if (UnityParticle->colorOverLifetime.enable)
			{
				SetScaleColorUnityGradient(ParticleUpdateScript, *VersionedEmitter->Emitter->GetUniqueEmitterName(), UnityParticle->colorOverLifetime.color);
			}
			if (UnityParticle->sizeOverLifetime.enable)
			{
				SetScaleSpriteSizeCurve(ParticleUpdateScript, *VersionedEmitter->Emitter->GetUniqueEmitterName(), UnityParticle->sizeOverLifetime.size);
			}
			if (UnityParticle->rendererData.enabled)
			{
				if (UnityParticle->rendererData.renderMode == TEXT("Stretch"))
				{
					SetScriptParamValue(ParticleUpdateScript,
										*FString::Printf(TEXT("Constants.%s.%s.Min Scale Factor"), *VersionedEmitter->Emitter->GetUniqueEmitterName(), *UnityParticle->rendererData.ScaleSpriteSizeBySpeedModuleName),
										FVector2f(1, UnityParticle->shape.length * m_ImportScale));
					SetScriptParamValue(ParticleUpdateScript,
										*FString::Printf(TEXT("Constants.%s.%s.Max Scale Factor"), *VersionedEmitter->Emitter->GetUniqueEmitterName(), *UnityParticle->rendererData.ScaleSpriteSizeBySpeedModuleName),
										FVector2f(1, UnityParticle->shape.length * m_ImportScale));
				}
				else if (UnityParticle->rendererData.renderMode == TEXT("HorizontalBillboard"))
				{
					SetScriptParamValue(ParticleSpawnScript,
										*FString::Printf(TEXT("Constants.%s.%s.Sprite Facing"), *VersionedEmitter->Emitter->GetUniqueEmitterName(), *UnityParticle->rendererData.SpriteFacingModuleName),
										FVector3f(0, 0, 1));
				}
			}
		}
	}

	NiagaraSystem->RequestCompile(false);
	NiagaraSystem->WaitForCompilationComplete();

	// 保存资产
	FAssetRegistryModule::AssetCreated(NiagaraSystem);
	NiagaraSystem->MarkPackageDirty();

	FString PackageFileName = FPackageName::LongPackageNameToFilename(FullAssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = EObjectFlags::RF_Standalone;
	bool bSaved = UPackage::SavePackage(Package, nullptr, *PackageFileName, SaveArgs);

	return NiagaraSystem;
}
UMaterial *SUnityParticleImportWindow::ImportMaterial(const FString &ContentDir, const FUPSMaterialData &UnityMaterial)
{
	if (UnityMaterial.mainTexture.IsEmpty())
		return nullptr;
	UTexture2D *Texture = ImportTexture(UnityMaterial.mainTexture, ContentDir);
	if (!Texture)
		return nullptr;

	return CreateAdditiveUnlitTwoSidedMaterialWithParticleColor(ContentDir, FPaths::GetBaseFilename(UnityMaterial.mainTexture), Texture);
}
UMaterial *SUnityParticleImportWindow::CreateAdditiveUnlitTwoSidedMaterial(const FString &ContentDir, const FString &MaterialName, UTexture2D *Texture)
{
	if (!Texture)
		return nullptr;

	// 创建包
	FString PackageName;
	FString AssetName;

	FAssetToolsModule::GetModule().Get().CreateUniqueAssetName(FPaths::Combine(ContentDir, MaterialName), "_mat", PackageName, AssetName);
	UPackage *Package = CreatePackage(*PackageName);
	if (!Package)
		return nullptr;
	Package->FullyLoad();

	// 创建材质对象
	UMaterialFactoryNew *MaterialFactory = NewObject<UMaterialFactoryNew>();
	UMaterial *Material = Cast<UMaterial>(MaterialFactory->FactoryCreateNew(UMaterial::StaticClass(), Package, FName(*AssetName), RF_Standalone | RF_Public, nullptr, GWarn));

	// 设置基本属性
	Material->BlendMode = BLEND_Additive;
	Material->SetShadingModel(MSM_Unlit);
	Material->TwoSided = true;

	// 创建 Texture Sample 节点
	UMaterialExpressionTextureSample *TextureSample = (UMaterialExpressionTextureSample *)UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionTextureSample::StaticClass());
	TextureSample->Texture = Texture;
	TextureSample->SamplerType = SAMPLERTYPE_Color;
	TextureSample->MaterialExpressionEditorX = -400;
	TextureSample->MaterialExpressionEditorY = 0;

	// 连接 Emissive Color（让它发光）
	UMaterialEditingLibrary::ConnectMaterialProperty(TextureSample, "RGB", EMaterialProperty::MP_EmissiveColor);
	UMaterialEditingLibrary::ConnectMaterialProperty(TextureSample, "A", EMaterialProperty::MP_Opacity);

	Material->PostEditChange();
	Package->MarkPackageDirty();
	// 保存资产
	FAssetRegistryModule::AssetCreated(Material);
	Material->MarkPackageDirty();

	// 通知编辑器
	if (GEditor) {
		if (UImportSubsystem* ImportSubsystem = GEditor->GetEditorSubsystem<UImportSubsystem>()) 
		{
			ImportSubsystem->OnAssetPostImport.Broadcast(MaterialFactory, Material);
		}
	}

	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = EObjectFlags::RF_Standalone;
	bool bSaved = UPackage::SavePackage(Package, nullptr, *PackageFileName, SaveArgs);

	return Material;
}
UMaterial *SUnityParticleImportWindow::CreateAdditiveUnlitTwoSidedMaterialWithParticleColor(const FString &ContentDir, const FString &MaterialName, UTexture2D *Texture)
{
	if (!Texture)
		return nullptr;

	// 创建包
	FString PackageName;
	FString AssetName;

	FAssetToolsModule::GetModule().Get().CreateUniqueAssetName(FPaths::Combine(ContentDir, MaterialName), "_mat", PackageName, AssetName);
	UPackage *Package = CreatePackage(*PackageName);
	if (!Package)
		return nullptr;
	Package->FullyLoad();

	// 创建材质对象
	UMaterialFactoryNew *MaterialFactory = NewObject<UMaterialFactoryNew>();
	UMaterial *Material = Cast<UMaterial>(MaterialFactory->FactoryCreateNew(UMaterial::StaticClass(), Package, FName(*AssetName), RF_Standalone | RF_Public, nullptr, GWarn));

	// 设置基本属性
	Material->BlendMode = BLEND_Additive;
	Material->SetShadingModel(MSM_Unlit);
	Material->TwoSided = true;

	// 创建 Texture Sample 节点
	UMaterialExpressionTextureSample *TextureSample = (UMaterialExpressionTextureSample *)UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionTextureSample::StaticClass());
	TextureSample->Texture = Texture;
	TextureSample->SamplerType = SAMPLERTYPE_Color;
	TextureSample->MaterialExpressionEditorX = -400;
	TextureSample->MaterialExpressionEditorY = 0;

	// 创建 Particle Color 节点
	UClass *ParticleColorClass = FindObject<UClass>(nullptr, TEXT("/Script/Engine.MaterialExpressionParticleColor"));
	if (!ParticleColorClass)
		return nullptr;
	UMaterialExpressionParticleColor *ParticleColor = (UMaterialExpressionParticleColor *)UMaterialEditingLibrary::CreateMaterialExpression(Material, ParticleColorClass);
	ParticleColor->MaterialExpressionEditorX = -400;
	ParticleColor->MaterialExpressionEditorY = 100;

	// 创建 Multiply 节点
	UMaterialExpressionMultiply *Multiply = (UMaterialExpressionMultiply *)UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionMultiply::StaticClass());
	Multiply->MaterialExpressionEditorX = -200;
	Multiply->MaterialExpressionEditorY = 50;

	// 创建 ALpha的Multiply 节点
	UMaterialExpressionMultiply *MultiplyForAlpha = (UMaterialExpressionMultiply *)UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionMultiply::StaticClass());
	MultiplyForAlpha->MaterialExpressionEditorX = -200;
	MultiplyForAlpha->MaterialExpressionEditorY = 100;

	// 连接节点
	// 将 TextureSample 的 RGB 输出连接到 Multiply 的 A 输入
	UMaterialEditingLibrary::ConnectMaterialExpressions(TextureSample, "RGB", Multiply, "A");
	// 将 ParticleColor 的输出连接到 Multiply 的 B 输入
	UMaterialEditingLibrary::ConnectMaterialExpressions(ParticleColor, "RGB", Multiply, "B");
	// 将 Multiply 的输出连接到自发光
	UMaterialEditingLibrary::ConnectMaterialProperty(Multiply, "", EMaterialProperty::MP_EmissiveColor);

	UMaterialEditingLibrary::ConnectMaterialExpressions(TextureSample, "A", MultiplyForAlpha, "A");
	UMaterialEditingLibrary::ConnectMaterialExpressions(ParticleColor, "A", MultiplyForAlpha, "B");
	// 连接 TextureSample 的 Alpha 到透明度
	UMaterialEditingLibrary::ConnectMaterialProperty(MultiplyForAlpha, "", EMaterialProperty::MP_Opacity);

	Material->PostEditChange();
	Material->MarkPackageDirty();
	// 保存资产
	FAssetRegistryModule::AssetCreated(Material);

	Package->MarkPackageDirty();

	// 通知编辑器
	if (GEditor) {
		if (UImportSubsystem* ImportSubsystem = GEditor->GetEditorSubsystem<UImportSubsystem>()) 
		{
			ImportSubsystem->OnAssetPostImport.Broadcast(MaterialFactory, Material);
		}
	}

	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = EObjectFlags::RF_Standalone;
	bool bSaved = UPackage::SavePackage(Package, nullptr, *PackageFileName, SaveArgs);

	return Material;
}

UTexture2D *SUnityParticleImportWindow::ImportTexture(const FString &ImageFilePath, const FString &ContentDir)
{
	if (!FPaths::FileExists(ImageFilePath))
	{
		UE_LOG(AnimationTools, Error, TEXT("贴图文件不存在: %s"), *ImageFilePath);
		return nullptr;
	}

	FString PackageName;
	FString AssetName;

	FAssetToolsModule::GetModule().Get().CreateUniqueAssetName(FPaths::Combine(ContentDir, FPaths::GetBaseFilename(ImageFilePath)), "_tex", PackageName, AssetName);

	UAssetImportTask *ImportTask = NewObject<UAssetImportTask>();
	ImportTask->Filename = ImageFilePath;
	ImportTask->DestinationPath = FPaths::GetPath(PackageName);
	ImportTask->DestinationName = AssetName;
	ImportTask->bAutomated = true;
	ImportTask->bSave = true;
	ImportTask->bReplaceExisting = true;

	FAssetToolsModule &AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	AssetToolsModule.Get().ImportAssetTasks({ImportTask});

	TArray<UObject *> Assets = ImportTask->GetObjects();
	for (UObject *pObj : Assets)
	{
		if (pObj->IsA<UTexture2D>())
			return Cast<UTexture2D>(pObj);
	}
	return nullptr;
}

FString SUnityParticleImportWindow::MakeUniqueContentDir(const FString &ContentDir, const FString &AssetName)
{
	FString Dir = FPaths::Combine(ContentDir, AssetName);
	int Num = 1;
	while (UEditorAssetLibrary::DoesDirectoryExist(Dir))
	{
		Dir = FPaths::Combine(ContentDir, FString::Printf(TEXT("%s_%d"), *AssetName, Num++));
	}
	return Dir;
}

UNiagaraNodeFunctionCall *SUnityParticleImportWindow::AddEmitterModule(FVersionedNiagaraEmitterData *EmitterData, ENiagaraScriptUsage ScriptUsage, const FString &ModuleAssetPath)
{
	UNiagaraScriptSource *ScriptSource = Cast<UNiagaraScriptSource>(EmitterData->GraphSource);
	UNiagaraGraph *Graph = ScriptSource ? ScriptSource->NodeGraph : nullptr;
	bool HaveExist = false;
	if(!ScriptSource->AddModuleIfMissing(ModuleAssetPath, ScriptUsage, HaveExist))
	{
		UE_LOG(AnimationTools, Error, TEXT("add module failed:%s"), *ModuleAssetPath);
		return nullptr;
	}

	TArray<UNiagaraNodeFunctionCall *> ModuleNodes;
	Graph->GetNodesOfClass<UNiagaraNodeFunctionCall>(ModuleNodes);

	UNiagaraNodeFunctionCall *FuncCallNode = NULL;
	for (UNiagaraNodeFunctionCall *ModuleNode : ModuleNodes)
	{
		if (ModuleNode->FunctionScriptAssetObjectPath == ModuleAssetPath)
		{
			return ModuleNode;
		}
	}
	return nullptr;
}

bool SUnityParticleImportWindow::InsertNodeBefore(UEdGraphNode *BeforeNode, const FString::ElementType *InsertPinName, UEdGraphNode *TargetNode, const FString::ElementType *FrontPinName, const FString::ElementType *BackPinName)
{
	UEdGraphPin *InsertPin = BeforeNode->FindPin(InsertPinName);
	if (!InsertPin)
		return false;
	if (InsertPin->LinkedTo.Num() <= 0)
		return false;
	UEdGraphPin *PrevPin = InsertPin->LinkedTo[0];
	UEdGraphPin *FrontPin = TargetNode->FindPin(FrontPinName);
	if (!FrontPin)
		return false;
	UEdGraphPin *BackPin = TargetNode->FindPin(BackPinName);
	if (!BackPin)
		return false;
	InsertPin->BreakLinkTo(PrevPin);
	FrontPin->MakeLinkTo(PrevPin);
	BackPin->MakeLinkTo(InsertPin);
	return true;
}

UNiagaraNodeFunctionCall *SUnityParticleImportWindow::AddInputTransModule(FVersionedNiagaraEmitterData *EmitterData, UNiagaraNodeFunctionCall *TargetModule, const FString::ElementType *ParamName, const FNiagaraTypeDefinition &ParamType, const FString &ModuleAssetPath, const FString &ModuleOutPinName)
{
	FSoftObjectPath SystemUpdateScriptRef(ModuleAssetPath);
	FAssetData ModuleScriptAsset;
	ModuleScriptAsset.PackageName = SystemUpdateScriptRef.GetAssetPath().GetPackageName();
	ModuleScriptAsset.AssetName = SystemUpdateScriptRef.GetAssetPath().GetAssetName();
	if (!ModuleScriptAsset.IsValid())
	{
		UE_LOG(AnimationTools, Error, TEXT("invalid module:%s"), *ModuleAssetPath);
		return nullptr;
	}		

	const UEdGraphSchema_Niagara *NiagaraSchema = GetDefault<UEdGraphSchema_Niagara>();
	UNiagaraScriptSource *ScriptSource = Cast<UNiagaraScriptSource>(EmitterData->GraphSource);
	UNiagaraGraph *Graph = ScriptSource ? ScriptSource->NodeGraph : nullptr;
	UClass *ParamMapSetNodeClass = FindObject<UClass>(nullptr, TEXT("/Script/Niagara.NiagaraNodeParameterMapSet"));
	if (!ParamMapSetNodeClass || !ParamMapSetNodeClass->IsChildOf(UNiagaraNode::StaticClass()))
		return nullptr;

	Graph->Modify();

	UEdGraphPin *TargetInputMapPin = TargetModule->FindPin(TEXT("InputMap"));
	if (!TargetInputMapPin)
		return nullptr;
	if (TargetInputMapPin->LinkedTo.Num() <= 0)
		return nullptr;

	UEdGraphPin *PrevPin = TargetInputMapPin->LinkedTo[0];
	UEdGraphNode *PrevNode = TargetInputMapPin->GetOwningNode();
	if (PrevNode == nullptr)
		return nullptr;

	UNiagaraNodeWithDynamicPins *ParamMapSetNode = nullptr;
	if (PrevNode->GetClass()->GetName() == TEXT("NiagaraNodeParameterMapSet"))
	{
		// 如果前置是ParamMapSetNode，直接插在这个ParamMapSetNode之前
		ParamMapSetNode = (UNiagaraNodeWithDynamicPins *)PrevNode;
		UEdGraphPin *InputMapPin = ParamMapSetNode->FindPin(TEXT("InputMap"));
		if (!InputMapPin)
			return nullptr;
		if (InputMapPin->LinkedTo.Num() <= 0)
			return nullptr;
		PrevPin = InputMapPin->LinkedTo[0];
	}
	else
	{
		// 需要创建一个ParamMapSetNode,插在中间
		FGraphNodeCreator<UEdGraphNode> ModuleNodeCreator(*Graph);
		ParamMapSetNode = (UNiagaraNodeWithDynamicPins *)ModuleNodeCreator.CreateNode(false, ParamMapSetNodeClass);
		ModuleNodeCreator.Finalize();
		if (!InsertNodeBefore(TargetModule, TEXT("InputMap"), ParamMapSetNode, TEXT("Source"), TEXT("Dest")))
			return nullptr;
	}

	UNiagaraNodeFunctionCall *NewModuleNode = nullptr;
	{
		FGraphNodeCreator<UNiagaraNodeFunctionCall> ModuleNodeCreator(*Graph);
		NewModuleNode = ModuleNodeCreator.CreateNode();
		PRAGMA_DISABLE_DEPRECATION_WARNINGS;
		NewModuleNode->FunctionScriptAssetObjectPath = FName(*ModuleScriptAsset.GetSoftObjectPath().ToString());
		PRAGMA_ENABLE_DEPRECATION_WARNINGS;
		ModuleNodeCreator.Finalize();
	}

	// 新模块的输入连接输入node
	UEdGraphPin *NewModuleInputMapPin = NewModuleNode->FindPin(TEXT("InputMap"));
	if (!NewModuleInputMapPin)
		return nullptr;
	NewModuleInputMapPin->MakeLinkTo(PrevPin);
	// 新模块的输出连接到ParamMapSetNode新建的参数Pin
	UEdGraphPin *NewModuleOutPin = NewModuleNode->FindPin(ModuleOutPinName);
	if (!NewModuleOutPin)
		return nullptr;
	FEdGraphPinType PinType = NiagaraSchema->TypeDefinitionToPinType(ParamType);
	FString PinName = FString::Printf(TEXT("%s.%s"), *TargetModule->FunctionScript->GetName(), ParamName);
	UEdGraphPin *ParamPin = ParamMapSetNode->CreatePin(EEdGraphPinDirection::EGPD_Input, PinType, *PinName);
	if (!ParamPin)
		return nullptr;
	NewModuleOutPin->MakeLinkTo(ParamPin);

	Graph->NotifyGraphChanged();
	Graph->MarkPackageDirty();
	return NewModuleNode;
}

UNiagaraNodeInput *SUnityParticleImportWindow::AddInputNode(FVersionedNiagaraEmitterData *EmitterData, UNiagaraNodeFunctionCall *TargetModule, const FString::ElementType *ParamName, const FNiagaraTypeDefinition &ParamType, UNiagaraDataInterface **DefaultDataInterface)
{
	const UEdGraphSchema_Niagara *NiagaraSchema = GetDefault<UEdGraphSchema_Niagara>();
	UNiagaraScriptSource *ScriptSource = Cast<UNiagaraScriptSource>(EmitterData->GraphSource);
	UNiagaraGraph *Graph = ScriptSource ? ScriptSource->NodeGraph : nullptr;
	UClass *ParamMapSetNodeClass = FindObject<UClass>(nullptr, TEXT("NiagaraNodeParameterMapSet"));
	if (!ParamMapSetNodeClass || !ParamMapSetNodeClass->IsChildOf(UNiagaraNode::StaticClass()))
		return nullptr;

	Graph->Modify();

	UEdGraphPin *TargetInputMapPin = TargetModule->FindPin(TEXT("InputMap"));
	if (!TargetInputMapPin)
		return nullptr;
	if (TargetInputMapPin->LinkedTo.Num() <= 0)
		return nullptr;

	UEdGraphPin *PreNodePin = TargetInputMapPin->LinkedTo[0];

	UNiagaraNodeWithDynamicPins *ParamMapSetNode = nullptr;
	if (PreNodePin->GetOwningNode()->GetClass()->GetName() == TEXT("NiagaraNodeParameterMapSet"))
	{
		// 如果前置节点是个ParamMapSet直接使用
		ParamMapSetNode = (UNiagaraNodeWithDynamicPins *)PreNodePin->GetOwningNode();
	}
	else
	{
		// 否则要创建个ParamMapSet插在二者之间
		FGraphNodeCreator<UEdGraphNode> ModuleNodeCreator(*Graph);
		ParamMapSetNode = (UNiagaraNodeWithDynamicPins *)ModuleNodeCreator.CreateNode(false, ParamMapSetNodeClass);
		ModuleNodeCreator.Finalize();
		if (!InsertNodeBefore(TargetModule, TEXT("InputMap"), ParamMapSetNode, TEXT("Source"), TEXT("Dest")))
			return nullptr;
	}
	// 创建参数Pin
	FEdGraphPinType PinType = NiagaraSchema->TypeDefinitionToPinType(ParamType);
	FString PinName = FString::Printf(TEXT("%s.%s"), *TargetModule->FunctionScript->GetName(), ParamName);
	UEdGraphPin *ParamPin = ParamMapSetNode->CreatePin(EEdGraphPinDirection::EGPD_Input, PinType, *PinName);
	if (!ParamPin)
		return nullptr;
	UNiagaraNodeInput *NewInputNode = nullptr;
	((UEdGraphSchema_Niagara *)NiagaraSchema)->PromoteSinglePinToParameter(ParamPin);
	if (ParamPin->LinkedTo.Num())
	{
		NewInputNode = Cast<UNiagaraNodeInput>(ParamPin->LinkedTo[0]->GetOwningNode());
		if (NewInputNode)
		{
			NewInputNode->Input.SetName(*PinName);
			if (DefaultDataInterface)
			{
				FProperty *Property = NewInputNode->GetClass()->FindPropertyByName(FName("DataInterface"));
				if (FObjectProperty *ObjectProperty = CastField<FObjectProperty>(Property))
				{
					*DefaultDataInterface = Cast<UNiagaraDataInterface>(ObjectProperty->GetObjectPropertyValue_InContainer(NewInputNode));
				}
			}
		}
	}

	Graph->NotifyGraphChanged();
	Graph->MarkPackageDirty();
	return NewInputNode;
}

UNiagaraNodeFunctionCall *SUnityParticleImportWindow::AddSubUVAnimationModule(FVersionedNiagaraEmitterData *EmitterData, UNiagaraSpriteRendererProperties *SpriteRenderer)
{
	const UEdGraphSchema_Niagara *NiagaraSchema = GetDefault<UEdGraphSchema_Niagara>();
	UNiagaraScriptSource *ScriptSource = Cast<UNiagaraScriptSource>(EmitterData->GraphSource);
	UNiagaraGraph *Graph = ScriptSource ? ScriptSource->NodeGraph : nullptr;
	Graph->Modify();

	UNiagaraNodeFunctionCall *ModuleNode = AddEmitterModule(EmitterData, ENiagaraScriptUsage::ParticleUpdateScript, TEXT("/Niagara/Modules/Update/SubUV/V2/SubUVAnimation.SubUVAnimation"));
	if (!ModuleNode)
		return nullptr;
	ModuleNode->MarkPackageDirty();

	FString PinName = FString::Printf(TEXT("%s.Sprite Renderer "), *ModuleNode->FunctionScript->GetName());

	UEdGraphPin *ModuleInputMapPin = ModuleNode->FindPin(TEXT("InputMap"));
	if (!ModuleInputMapPin)
		return nullptr;
	UNiagaraNodeFunctionCall *PrevModule = nullptr;
	UEdGraphPin *PrevModuleOutputPin = nullptr;
	if (ModuleInputMapPin->LinkedTo.Num())
	{
		PrevModuleOutputPin = ModuleInputMapPin->LinkedTo[0];
		PrevModule = Cast<UNiagaraNodeFunctionCall>(PrevModuleOutputPin->GetOwningNode());
	}
	// for (UEdGraphPin* LinkPin : ModuleInputMapPin->LinkedTo)
	//{
	//	UEdGraphNode* LinkNode = LinkPin->GetOwningNode();
	//	if (auto LinkModule = Cast<UNiagaraNodeFunctionCall>(LinkNode))
	//	{
	//		if (LinkModule->FunctionScript->GetName() == "ParticleState")
	//		{
	//			PrevModule = LinkModule;
	//			PrevModuleOutputPin = LinkPin;
	//			ModuleInputMapPin->BreakLinkTo(LinkPin);
	//			break;
	//		}
	//	}
	// }

	if (!PrevModule || !PrevModuleOutputPin)
		return nullptr;

	UNiagaraNodeWithDynamicPins *ParamMapSetNode = nullptr;
	{
		UClass *NodeClass = FindObject<UClass>(nullptr, TEXT("NiagaraNodeParameterMapSet"));
		if (!NodeClass || !NodeClass->IsChildOf(UNiagaraNode::StaticClass()))
			return nullptr;
		FGraphNodeCreator<UEdGraphNode> ModuleNodeCreator(*Graph);
		ParamMapSetNode = (UNiagaraNodeWithDynamicPins *)ModuleNodeCreator.CreateNode(false, NodeClass);
		if (!ParamMapSetNode)
			return nullptr;
		ModuleNodeCreator.Finalize();
	}
	UEdGraphPin *SourcePin = ParamMapSetNode->FindPin(TEXT("Source"));
	if (!SourcePin)
		return nullptr;
	PrevModuleOutputPin->BreakLinkTo(PrevModuleOutputPin->LinkedTo[0]);
	SourcePin->MakeLinkTo(PrevModuleOutputPin);

	UEdGraphPin *DestPin = ParamMapSetNode->FindPin(TEXT("Dest"));
	if (!DestPin)
		return nullptr;
	DestPin->MakeLinkTo(ModuleInputMapPin);

	FEdGraphPinType PinType = NiagaraSchema->TypeDefinitionToPinType(FNiagaraTypeDefinition(UNiagaraDataInterfaceSpriteRendererInfo::StaticClass()));

	UEdGraphPin *ParamPin = ParamMapSetNode->CreatePin(EEdGraphPinDirection::EGPD_Input, PinType, *PinName);
	if (!ParamPin)
		return nullptr;

	((UEdGraphSchema_Niagara *)NiagaraSchema)->PromoteSinglePinToParameter(ParamPin);
	// ParamPin->MakeLinkTo(InputInputPin);

	UNiagaraDataInterfaceSpriteRendererInfo *RendererInfo = nullptr;
	if (ParamPin->LinkedTo.Num())
	{
		UNiagaraNodeInput *InputNode = Cast<UNiagaraNodeInput>(ParamPin->LinkedTo[0]->GetOwningNode());
		if (InputNode)
		{
			InputNode->Input.SetName(*PinName);
			FProperty *Property = InputNode->GetClass()->FindPropertyByName(FName("DataInterface"));
			if (FObjectProperty *ObjectProperty = CastField<FObjectProperty>(Property))
			{
				RendererInfo = Cast<UNiagaraDataInterfaceSpriteRendererInfo>(ObjectProperty->GetObjectPropertyValue_InContainer(InputNode));
			}
		}
	}

	if (RendererInfo)
	{
		UClass *InfoClass = UNiagaraDataInterfaceSpriteRendererInfo::StaticClass();
		FNiagaraTypeDefinition VarType(InfoClass);
		FProperty *SpriteRendererProp = FindFProperty<FProperty>(InfoClass, TEXT("SpriteRenderer"));
		if (SpriteRendererProp)
		{
			void *PropertyPtr = SpriteRendererProp->ContainerPtrToValuePtr<void>(RendererInfo);
			if (FObjectProperty *ObjectProp = CastField<FObjectProperty>(SpriteRendererProp))
			{
				ObjectProp->SetObjectPropertyValue(PropertyPtr, SpriteRenderer);
				RendererInfo->MarkPackageDirty();
			}
		}
	}

	Graph->NotifyGraphChanged();
	Graph->MarkPackageDirty();
	return ModuleNode;
}

bool SUnityParticleImportWindow::SetEmitterModulePin(UNiagaraNodeFunctionCall *Node, const FString &PinName, const FString &Value)
{
	UEdGraphPin *Pin = Node->FindPin(*PinName);
	if (Pin)
	{
		Pin->DefaultValue = Value;
		Node->PinDefaultValueChanged(Pin);
		return true;
	}
	return false;
}

bool SUnityParticleImportWindow::SetEmitterModulePinWithEnum(UNiagaraNodeFunctionCall *Node, const FString &PinName, const FString &EnumName, int64 Value)
{
	UEnum *Enum = LoadObject<UEnum>(nullptr, *EnumName);
	if (Enum)
	{
		FString ValueStr;
		if (Enum->FindNameStringByValue(ValueStr, Value))
			return SetEmitterModulePin(Node, PinName, ValueStr);
	}
	else
	{
		UE_LOG(AnimationTools, Error, TEXT("can`t find enum:%s"), *EnumName);
	}
	return false;
}

bool SUnityParticleImportWindow::HaveValue(const FUPSSerializableCurve &Value)
{
	if (Value.mode == TEXT("Constant"))
	{
		return Value.constantMax != 0;
	}
	else if (Value.mode == TEXT("TwoConstants"))
	{
		return Value.constantMin != 0 || Value.constantMax != 0;
	}
	else if (Value.mode == TEXT("Curve"))
	{
		return Value.curveMax.keys.Num() > 0;
	}
	else if (Value.mode == TEXT("TwoCurves"))
	{
		return Value.curveMax.keys.Num() > 0 || Value.curveMin.keys.Num() > 0;
	}
	return false;
}

bool SUnityParticleImportWindow::SetupModuleFloatCurveParam(FVersionedNiagaraEmitterData *EmitterData, UNiagaraNodeFunctionCall *Module, const FString::ElementType *ParamName, FUPSSerializableCurve &UnityCurve)
{
	if (UnityCurve.mode == TEXT("TwoConstants"))
	{
		UNiagaraNodeFunctionCall *InputTransNode = AddInputTransModule(EmitterData, Module, ParamName, FNiagaraTypeDefinition::GetFloatDef(),
																	   TEXT("/Niagara/DynamicInputs/UniformRange/V2/RandomRangeFloat.RandomRangeFloat"), TEXT("UniformRangedFloat"));
		if (InputTransNode)
		{
			UnityCurve.BindModuleName = InputTransNode->GetFunctionName();
			return true;
		}
		return false;
	}
	return true;
}

bool SUnityParticleImportWindow::SetupModuleSpeedCurveParam(FVersionedNiagaraEmitterData *EmitterData, UNiagaraNodeFunctionCall *Module, const FString::ElementType *ParamName, FUPSSerializableCurve &UnityCurve)
{
	if (UnityCurve.mode == TEXT("TwoConstants"))
	{
		UNiagaraNodeFunctionCall *InputTransNode = AddInputTransModule(EmitterData, Module, ParamName, FNiagaraTypeDefinition::GetFloatDef(),
																	   TEXT("/Niagara/DynamicInputs/UniformRange/V2/RandomRangeVector.RandomRangeVector"), TEXT("NewOutput"));
		if (InputTransNode)
		{
			UnityCurve.BindModuleName = InputTransNode->GetFunctionName();
			return true;
		}
		return false;
	}
	return true;
}

bool SUnityParticleImportWindow::SetupScaleColorModule(FVersionedNiagaraEmitterData *EmitterData, UNiagaraNodeFunctionCall *Module, FUPSSerializableMinMaxGradient &UnityGradient)
{
	if (UnityGradient.mode == TEXT("TwoColors") || UnityGradient.mode == TEXT("Gradient"))
	{
		SetEmitterModulePinWithEnum(Module, TEXT("Scale Mode"), TEXT("/Niagara/Enums/ENiagaraScaleColorMode.ENiagaraScaleColorMode"), 2);
		UNiagaraDataInterfaceColorCurve *DataInterface = nullptr;
		UNiagaraNodeInput *InputNode = AddInputNode(EmitterData, Module, TEXT("Linear Color Curve"), UNiagaraDataInterfaceColorCurve::StaticClass(), (UNiagaraDataInterface **)(&DataInterface));
		if (InputNode)
		{
			if (DataInterface && DataInterface->GetClass()->GetName() == TEXT("NiagaraDataInterfaceColorCurve"))
			{
				DataInterface->RedCurve.Reset();
				DataInterface->GreenCurve.Reset();
				DataInterface->BlueCurve.Reset();
				DataInterface->AlphaCurve.Reset();
				if (UnityGradient.mode == TEXT("Gradient"))
				{
					for (auto &Frame : UnityGradient.gradientMax.colorKeys)
					{
						auto Color = ColorStr2LinearColor(Frame.color);
						auto Handle = DataInterface->RedCurve.AddKey(Frame.time, Color.R);
						DataInterface->RedCurve.SetKeyInterpMode(Handle, ERichCurveInterpMode::RCIM_Linear);
						Handle = DataInterface->GreenCurve.AddKey(Frame.time, Color.G);
						DataInterface->GreenCurve.SetKeyInterpMode(Handle, ERichCurveInterpMode::RCIM_Linear);
						Handle = DataInterface->BlueCurve.AddKey(Frame.time, Color.B);
						DataInterface->BlueCurve.SetKeyInterpMode(Handle, ERichCurveInterpMode::RCIM_Linear);
					}
					for (auto &Frame : UnityGradient.gradientMax.alphaKeys)
					{
						auto Handle = DataInterface->AlphaCurve.AddKey(Frame.time, Frame.alpha);
						DataInterface->AlphaCurve.SetKeyInterpMode(Handle, ERichCurveInterpMode::RCIM_Linear);
					}
				}
				else
				{
					auto Color = ColorStr2LinearColor(UnityGradient.colorMin);
					auto Handle = DataInterface->RedCurve.AddKey(0, Color.R);
					DataInterface->RedCurve.SetKeyInterpMode(Handle, ERichCurveInterpMode::RCIM_Linear);
					Handle = DataInterface->GreenCurve.AddKey(0, Color.G);
					DataInterface->GreenCurve.SetKeyInterpMode(Handle, ERichCurveInterpMode::RCIM_Linear);
					Handle = DataInterface->BlueCurve.AddKey(0, Color.B);
					DataInterface->BlueCurve.SetKeyInterpMode(Handle, ERichCurveInterpMode::RCIM_Linear);
					Handle = DataInterface->AlphaCurve.AddKey(0, Color.A);
					DataInterface->AlphaCurve.SetKeyInterpMode(Handle, ERichCurveInterpMode::RCIM_Linear);
					Color = ColorStr2LinearColor(UnityGradient.colorMax);
					Handle = DataInterface->RedCurve.AddKey(1, Color.R);
					DataInterface->RedCurve.SetKeyInterpMode(Handle, ERichCurveInterpMode::RCIM_Linear);
					Handle = DataInterface->GreenCurve.AddKey(1, Color.G);
					DataInterface->GreenCurve.SetKeyInterpMode(Handle, ERichCurveInterpMode::RCIM_Linear);
					Handle = DataInterface->BlueCurve.AddKey(1, Color.B);
					DataInterface->BlueCurve.SetKeyInterpMode(Handle, ERichCurveInterpMode::RCIM_Linear);
					Handle = DataInterface->AlphaCurve.AddKey(1, Color.A);
					DataInterface->AlphaCurve.SetKeyInterpMode(Handle, ERichCurveInterpMode::RCIM_Linear);
				}
			}
			else
			{
				UE_LOG(AnimationTools, Warning, TEXT("set input DataInterface failed on module:%s"), *Module->GetFunctionName());
			}
			return true;
		}
		return false;
	}
	else
	{
		SetEmitterModulePinWithEnum(Module, TEXT("Scale Mode"), TEXT("/Niagara/Enums/ENiagaraScaleColorMode.ENiagaraScaleColorMode"), 1);
		SetEmitterModulePin(Module, TEXT("ScaleRGBA"), LexToSanitizedString(true));
	}
	return true;
}

bool SUnityParticleImportWindow::SetupScaleSpriteSizeModule(FVersionedNiagaraEmitterData *EmitterData, UNiagaraNodeFunctionCall *Module, const FUPSSerializableCurve &Value)
{
	if (Value.mode == TEXT("TwoConstants") || Value.mode == TEXT("Curve"))
	{
		SetEmitterModulePinWithEnum(Module, TEXT("Scale Sprite Size Mode"), TEXT("/Niagara/Enums/SpriteRenderer/ENiagara_ScaleSpriteSize.ENiagara_ScaleSpriteSize"), 1);
		UNiagaraDataInterfaceCurve *DataInterface = nullptr;
		UNiagaraNodeInput *InputNode = AddInputNode(EmitterData, Module, TEXT("Uniform Curve Sprite Scale"), UNiagaraDataInterfaceCurve::StaticClass(), (UNiagaraDataInterface **)(&DataInterface));
		if (InputNode)
		{
			if (DataInterface && DataInterface->GetClass()->GetName() == TEXT("NiagaraDataInterfaceCurve"))
			{
				DataInterface->Curve.Reset();
				if (Value.mode == TEXT("Curve"))
				{
					TArray<FRichCurveKey> Keys;
					Keys.Reserve(Value.curveMax.keys.Num());
					for (auto &Frame : Value.curveMax.keys)
					{
						FRichCurveKey &Key = Keys.AddDefaulted_GetRef();
						Key.Time = Frame.time;
						Key.Value = Frame.value;
						Key.InterpMode = ERichCurveInterpMode::RCIM_Cubic;
						Key.TangentMode = ERichCurveTangentMode::RCTM_User;
						Key.TangentWeightMode = ERichCurveTangentWeightMode::RCTWM_WeightedBoth;
						Key.ArriveTangent = Frame.inTangent;
						Key.ArriveTangentWeight = Frame.inWeight;
						Key.LeaveTangent = Frame.outTangent;
						Key.LeaveTangentWeight = Frame.outWeight;
					}
					DataInterface->Curve.SetKeys(Keys);
				}
				else
				{
					DataInterface->Curve.Reset();
					auto Handle = DataInterface->Curve.AddKey(0, Value.constantMin);
					DataInterface->Curve.SetKeyInterpMode(Handle, ERichCurveInterpMode::RCIM_Linear);
					Handle = DataInterface->Curve.AddKey(1, Value.constantMax);
					DataInterface->Curve.SetKeyInterpMode(Handle, ERichCurveInterpMode::RCIM_Linear);
				}
			}
			else
			{
				UE_LOG(AnimationTools, Warning, TEXT("set input DataInterface failed on module:%s"), *Module->GetFunctionName());
			}
			return true;
		}
		return false;
	}
	else
	{
		SetEmitterModulePinWithEnum(Module, TEXT("Scale Sprite Size Mode"), TEXT("/Niagara/Enums/SpriteRenderer/ENiagara_ScaleSpriteSize.ENiagara_ScaleSpriteSize"), 0);
	}
	return true;
}

void SUnityParticleImportWindow::SetScriptFloatParamUnityCurve(UNiagaraScript *Script, const FString::ElementType *EmitterName, const FString::ElementType *ParamName, const FUPSSerializableCurve &Value, float Scale, const FString::ElementType *ModuleName)
{
	if (!ModuleName)
		ModuleName = *Value.BindModuleName;
	if (Value.mode == TEXT("TwoConstants"))
	{
		SetScriptParamValue(Script, *FString::Printf(TEXT("Constants.%s.%s.Minimum"), EmitterName, ModuleName), Value.constantMin * Scale);
		SetScriptParamValue(Script, *FString::Printf(TEXT("Constants.%s.%s.Maximum"), EmitterName, ModuleName), Value.constantMax * Scale);
	}
	else
	{
		SetScriptParamValue(Script, *FString::Printf(TEXT("Constants.%s.%s.%s"), EmitterName, ModuleName, ParamName), Value.constantMax * Scale);
	}
}

void SUnityParticleImportWindow::SetScriptIntParamUnityCurve(UNiagaraScript *Script, const FString::ElementType *EmitterName, const FString::ElementType *ParamName, const FUPSSerializableCurve &Value, float Scale, const FString::ElementType *ModuleName)
{
	if (!ModuleName)
		ModuleName = *Value.BindModuleName;
	if (Value.mode == TEXT("TwoConstants"))
	{
		SetScriptParamValue(Script, *FString::Printf(TEXT("Constants.%s.%s.Minimum"), EmitterName, ModuleName), (int)(Value.constantMin * Scale));
		SetScriptParamValue(Script, *FString::Printf(TEXT("Constants.%s.%s.Maximum"), EmitterName, ModuleName), (int)(Value.constantMax * Scale));
	}
	else
	{
		SetScriptParamValue(Script, *FString::Printf(TEXT("Constants.%s.%s.%s"), EmitterName, ModuleName, ParamName), (int)(Value.constantMax * Scale));
	}
}

void SUnityParticleImportWindow::SetScriptSpeedParamUnityCurve(UNiagaraScript *Script, const FString::ElementType *EmitterName, const FString::ElementType *ParamName, const FUPSSerializableCurve &Value, float Scale, const FQuat4f &Rotation, const FString::ElementType *ModuleName)
{
	if (!ModuleName)
		ModuleName = *Value.BindModuleName;
	if (Value.mode == TEXT("TwoConstants"))
	{
		SetScriptParamValue(Script, *FString::Printf(TEXT("Constants.%s.%s.Minimum"), EmitterName, ModuleName), Rotation.RotateVector(FVector3f(Value.constantMin, 0, 0) * Scale));
		SetScriptParamValue(Script, *FString::Printf(TEXT("Constants.%s.%s.Maximum"), EmitterName, ModuleName), Rotation.RotateVector(FVector3f(Value.constantMax, 0, 0) * Scale));
	}
	else
	{
		SetScriptParamValue(Script, *FString::Printf(TEXT("Constants.%s.%s.%s"), EmitterName, ModuleName, ParamName), Rotation.RotateVector(FVector3f(Value.constantMax, 0, 0) * Scale));
	}
}

void SUnityParticleImportWindow::SetScaleColorUnityGradient(UNiagaraScript *Script, const FString::ElementType *EmitterName, const FUPSSerializableMinMaxGradient &Value)
{
	if (Value.mode == TEXT("TwoColors") || Value.mode == TEXT("Gradient"))
	{
	}
	else
	{
		SetScriptParamValue(Script, *FString::Printf(TEXT("Constants.%s.%s.Scale RGBA"), EmitterName, *Value.BindModuleName), ColorStr2LinearColor(Value.colorMax));
	}
}

void SUnityParticleImportWindow::SetScaleSpriteSizeCurve(UNiagaraScript *Script, const FString::ElementType *EmitterName, const FUPSSerializableCurve &Value)
{
	if (Value.mode == TEXT("TwoConstants") || Value.mode == TEXT("Curve"))
	{
	}
	else
	{
		SetScriptParamValue(Script, *FString::Printf(TEXT("Constants.%s.%s.Uniform Scale Factor"), EmitterName, *Value.BindModuleName), Value.constantMax);
	}
}

void SUnityParticleImportWindow::DumpNiagaraSystem(UNiagaraSystem *NiagaraSystem)
{
	if (NiagaraSystem)
	{
		UE_LOG(AnimationTools, Error, TEXT("NiagaraSystem:%s"), *NiagaraSystem->GetFullName());
		FString Params = NiagaraSystem->GetExposedParameters().ToString();
		if (!Params.IsEmpty())
		{
			TArray<FString> ParamList;
			Params.ParseIntoArrayLines(ParamList);
			for (FString &Param : ParamList)
			{
				UE_LOG(AnimationTools, Error, TEXT("    Params:%s"), *Param);
			}
		}

		for (int i = 0; i < NiagaraSystem->GetNumEmitters(); i++)
		{
			FNiagaraEmitterHandle &Handle = NiagaraSystem->GetEmitterHandle(i);
			const FVersionedNiagaraEmitter& VersionedEmitterRef = Handle.GetInstance();
			const FVersionedNiagaraEmitter *VersionedEmitter = &VersionedEmitterRef;
			if (VersionedEmitter)
			{
				UE_LOG(AnimationTools, Error, TEXT("Emitter:%s"), *VersionedEmitter->Emitter->GetUniqueEmitterName());
				FVersionedNiagaraEmitterData *EmitterData = const_cast<FVersionedNiagaraEmitterData*>(VersionedEmitter->GetEmitterData());
				for (auto Renderer : EmitterData->GetRenderers())
				{
					UE_LOG(AnimationTools, Error, TEXT("    Renderer:%s(%p)"), *Renderer->GetClass()->GetName(), Renderer);
				}
				UNiagaraScriptSource *ScriptSource = Cast<UNiagaraScriptSource>(EmitterData->GraphSource);
				TObjectPtr<UNiagaraGraph> NodeGraph = ScriptSource->NodeGraph;
				DumpGraph(NodeGraph, TEXT(""), 5);
				TArray<UNiagaraScript *> Scripts;
				EmitterData->GetScripts(Scripts, false);
				for (UNiagaraScript *Script : Scripts)
				{
					DumpScript(Script, TEXT(""));
				}
			}
		}
	}
}
void SUnityParticleImportWindow::DumpGraph(TObjectPtr<UNiagaraGraph> Graph, FString Space, int Deep)
{
	if (!IsValid(Graph))
		return;
	UE_LOG(AnimationTools, Error, TEXT("%sGraph:%s"), *Space, *Graph->GetFullName());
	Space += TEXT("    ");
	for (TObjectPtr<UEdGraphNode> Node : Graph->Nodes)
	{
		DumpNode(Node, Space);
		if (Deep > 0)
		{
			auto SubGraphs = Node->GetSubGraphs();
			for (auto SubGraph : SubGraphs)
				DumpGraph(Cast<UNiagaraGraph>(SubGraph), Space, Deep - 1);
		}
	}
	UNiagaraGraph::FScriptVariableMap &VariableMap = Graph->GetAllMetaData();
	for (auto &Pair : VariableMap)
	{
		UE_LOG(AnimationTools, Error, TEXT("%sScriptVar:%s(%s)"), *Space, *Pair.Key.GetName().ToString(), *Pair.Key.GetType().GetName());
	}
}
void SUnityParticleImportWindow::DumpNode(TObjectPtr<UEdGraphNode> Node, FString Space)
{
	if (auto *FuncNode = Cast<UNiagaraNodeFunctionCall>(Node))
	{
		UE_LOG(AnimationTools, Error, TEXT("%sModule(%s):%s->%s %s"), *Space,
			   IsValid(FuncNode->FunctionScript) ? *FuncNode->FunctionScript->GetName() : TEXT("null"),
			   *FuncNode->GetName(),
			   IsValid(FuncNode->FunctionScript) ? *FuncNode->FunctionScript->GetFullName() : TEXT("null"),
			   *FuncNode->GetFunctionName());
	}
	// else if (auto* ParamNode = Cast<UNiagaraNodeParameterMapSet>(Node))
	//{
	//	UE_LOG(AnimationTools, Error, TEXT("%sParameterMapSet:%s"), *Space, *Node->GetName());
	// }
	else if (auto *InputNode = Cast<UNiagaraNodeInput>(Node))
	{
		UE_LOG(AnimationTools, Error, TEXT("%sInput(%s:%d):%s|%s"), *Space, *InputNode->Input.GetType().GetName(), InputNode->Input.GetAllocatedSizeInBytes(), *Node->GetName(), *InputNode->Input.GetName().ToString());
	}
	else if (auto *OutputNode = Cast<UNiagaraNodeOutput>(Node))
	{
		auto Enum = StaticEnum<ENiagaraScriptUsage>();
		UE_LOG(AnimationTools, Error, TEXT("%sOutput(%s):%s"), *Space, *Enum->GetNameStringByValue((int64)OutputNode->GetUsage()), *Node->GetName());
	}
	else
	{
		UE_LOG(AnimationTools, Error, TEXT("%sNode(%s);%s"), *Space, *Node->GetClass()->GetName(), *Node->GetName());
	}
	Space += TEXT("    ");
	for (UEdGraphPin *Pin : Node->Pins)
	{
		DumpPin(Pin, Space);
	}
}

void SUnityParticleImportWindow::DumpPin(UEdGraphPin *Pin, FString Space)
{
	const UEnum *Enum = StaticEnum<EEdGraphPinDirection>();

	UE_LOG(AnimationTools, Error, TEXT("%sPin(%s):%s,Type:%s|%s|%s=%s"), *Space, *Enum->GetNameStringByValue((int64)Pin->Direction),
		   *Pin->GetName(), *Pin->PinType.PinCategory.ToString(), *Pin->PinType.PinSubCategory.ToString(),
		   Pin->PinType.PinSubCategoryObject.IsValid() ? *Pin->PinType.PinSubCategoryObject->GetFullName() : TEXT("null"),
		   IsValid(Pin->DefaultObject) ? *Pin->DefaultObject->GetName() : *Pin->DefaultValue);
	Space += TEXT("    ");
	for (UEdGraphPin *LinkPin : Pin->LinkedTo)
	{
		FString Parent;
		UEdGraphNode *ParentNode = LinkPin->GetOwningNode();
		if (ParentNode)
		{
			if (auto *FuncNode = Cast<UNiagaraNodeFunctionCall>(ParentNode))
				Parent = FString::Printf(_TEXT("(%s)%s"), IsValid(FuncNode->FunctionScript) ? *FuncNode->FunctionScript->GetName() : TEXT("null"), *FuncNode->GetName());
			else
				Parent = FString::Printf(_TEXT("(%s)%s"), *ParentNode->GetClass()->GetName(), *ParentNode->GetName());
		}
		UE_LOG(AnimationTools, Error, TEXT("%sLink:%s.%s"), *Space, *Parent, *LinkPin->GetName());
	}
}

void SUnityParticleImportWindow::DumpScript(UNiagaraScript *Script, FString Space)
{
	const UEnum *Enum = StaticEnum<ENiagaraScriptUsage>();
	UE_LOG(AnimationTools, Error, TEXT("%sScript(%s):%s"), *Space, *Enum->GetNameStringByValue((int64)Script->GetUsage()), *Script->GetName());
	FString Params = Script->RapidIterationParameters.ToString();
	if (!Params.IsEmpty())
	{
		FString ParamSpace = Space + TEXT("    ");
		TArray<FString> ParamList;
		Params.ParseIntoArrayLines(ParamList);
		for (FString &Param : ParamList)
		{
			UE_LOG(AnimationTools, Error, TEXT("%sParams:%s"), *ParamSpace, *Param);
		}
	}
	Params = Script->RapidIterationParametersCookedEditorCache.ToString();
	if (!Params.IsEmpty())
	{
		FString ParamSpace = Space + TEXT("    ");
		TArray<FString> ParamList;
		Params.ParseIntoArrayLines(ParamList);
		for (FString &Param : ParamList)
		{
			UE_LOG(AnimationTools, Error, TEXT("%sCookedParams:%s"), *ParamSpace, *Param);
		}
	}
	{
		auto DataInterfaces = Script->GetCachedDefaultDataInterfaces();
		FString ParamSpace = Space + TEXT("    ");
		for (auto &DataInterface : DataInterfaces)
		{
			FString Value(TEXT("Unknow"));
			uint32 InfoUniqueID = 0;
			FString InfoClass;
			if (DataInterface.DataInterface)
			{
				InfoClass = DataInterface.DataInterface->GetClass()->GetName();
				InfoUniqueID = DataInterface.DataInterface->GetUniqueID();
			}
			if (UNiagaraDataInterfaceSpriteRendererInfo *RendererInfo = Cast<UNiagaraDataInterfaceSpriteRendererInfo>(DataInterface.DataInterface))
			{
				if (RendererInfo->GetSpriteRenderer())
					Value = FString::Printf(TEXT("%s(%p)"), *RendererInfo->GetSpriteRenderer()->GetClass()->GetName(), RendererInfo->GetSpriteRenderer());
				else
					Value = TEXT("null");
			}
			else if (UNiagaraDataInterfaceColorCurve *CurveInfo = Cast<UNiagaraDataInterfaceColorCurve>(DataInterface.DataInterface))
			{
				Value = FString::Printf(TEXT("R(%d)G(%d)B(%d)A(%d)"), CurveInfo->RedCurve.GetNumKeys(), CurveInfo->GreenCurve.GetNumKeys(), CurveInfo->BlueCurve.GetNumKeys(), CurveInfo->AlphaCurve.GetNumKeys());
			}
			UE_LOG(AnimationTools, Error, TEXT("%sResolvedDataInterface:%s(%s)=(%d)%s"), *ParamSpace, *DataInterface.Name.ToString(), *InfoClass, InfoUniqueID, *Value);
		}
	}
}

void SUnityParticleImportWindow::OpenNiagaraScriptGraphViewer(UNiagaraGraph *NiagaraGraph, const FString::ElementType *GraphName)
{
	FName TabId = FName(GraphName);

	// 注册 tab，如果已经存在就不会重复注册
	if (!FGlobalTabmanager::Get()->HasTabSpawner(TabId))
	{
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TabId, FOnSpawnTab::CreateLambda(
																	 [NiagaraGraph, GraphName](const FSpawnTabArgs &)
																	 {
																		 FGraphAppearanceInfo AppearanceInfo;
																		 AppearanceInfo.CornerText = LOCTEXT("NiagaraGraphCornerText", "Niagara Graph");

																		 TSharedRef<SGraphEditor> GraphEditor = SNew(SGraphEditor)
																													.GraphToEdit(NiagaraGraph)
																													.Appearance(AppearanceInfo)
																													.GraphEvents(SGraphEditor::FGraphEditorEvents())
																													.AutoExpandActionMenu(false);

																		 return SNew(SDockTab)
																			 .TabRole(ETabRole::NomadTab)
																			 .Label(FText::FromString(GraphName))
																				 [GraphEditor];
																	 }))
			.SetDisplayName(FText::FromString(GraphName))
			.SetMenuType(ETabSpawnerMenuType::Hidden);
	}

	// 打开 Tab
	FGlobalTabmanager::Get()->TryInvokeTab(TabId);
}

void SUnityParticleImportWindow::ShowNiagaraSystemGraph(UNiagaraSystem *NiagaraSystem)
{
	if (NiagaraSystem)
	{
		UE_LOG(AnimationTools, Error, TEXT("NiagaraSystem:%s"), *NiagaraSystem->GetFullName());
		FString Params = NiagaraSystem->GetExposedParameters().ToString();
		if (!Params.IsEmpty())
		{
			TArray<FString> ParamList;
			Params.ParseIntoArrayLines(ParamList);
			for (FString &Param : ParamList)
			{
				UE_LOG(AnimationTools, Error, TEXT("    Params:%s"), *Param);
			}
		}

		for (int i = 0; i < NiagaraSystem->GetNumEmitters(); i++)
		{
			FNiagaraEmitterHandle &Handle = NiagaraSystem->GetEmitterHandle(i);
			const FVersionedNiagaraEmitter& VersionedEmitterRef = Handle.GetInstance();
			const FVersionedNiagaraEmitter *VersionedEmitter = &VersionedEmitterRef;
			if (VersionedEmitter)
			{
				FVersionedNiagaraEmitterData *EmitterData = const_cast<FVersionedNiagaraEmitterData*>(VersionedEmitter->GetEmitterData());
				UNiagaraScriptSource *ScriptSource = Cast<UNiagaraScriptSource>(EmitterData->GraphSource);
				TObjectPtr<UNiagaraGraph> NodeGraph = ScriptSource->NodeGraph;
				OpenNiagaraScriptGraphViewer(NodeGraph, *VersionedEmitter->Emitter->GetUniqueEmitterName());
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
