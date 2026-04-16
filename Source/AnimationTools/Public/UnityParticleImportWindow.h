#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWindow.h"
#include "Widgets/Views/SListView.h"

#include "UnityParticleSystem.h"
#include "NiagaraSystem.h"
#include "NiagaraNodeFunctionCall.h"
#include "AnimationTools.h"

class UNiagaraSpriteRendererProperties;

class SUnityParticleImportWindow : public SWindow
{
public:
	SLATE_BEGIN_ARGS(SUnityParticleImportWindow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	static void OpenWindow();
	static void DumpNiagaraSystem(UNiagaraSystem* NiagaraSystem);
	static void ShowNiagaraSystemGraph(UNiagaraSystem* NiagaraSystem);
private:
	TSharedPtr<SEditableTextBox> m_JsonPathTextBox;
	TSharedPtr<SEditableTextBox> m_ContentPathTextBox;
	TSharedPtr<SListView<TSharedPtr<FUPSParticleSystemData>>> m_EmitterListView;
	TArray<TSharedPtr<FUPSParticleSystemData>> m_EmitterList;
	float m_ImportScale = 20.0f;
	bool m_AdjustEmitterDuration = true;


	FReply OnBrowseJsonPath();
	FReply OnBrowseContentPath();
	void OnTargetDirSelected(const FString& SelectDir);
	FReply OnLoadJson();
	FReply OnImportParticles();

	TSharedRef<ITableRow> GenerateEmitterRow(TSharedPtr<FUPSParticleSystemData> Item, const TSharedRef<STableViewBase>& OwnerTable);
	void RefreshEmitterList();
	UNiagaraSystem* CreateNiagaraSystem();
	UMaterial* ImportMaterial(const FString& ContentDir, const FUPSMaterialData& UnityMaterial);
	UMaterial* CreateAdditiveUnlitTwoSidedMaterial(const FString& ContentDir, const FString& MaterialName, UTexture2D* Texture);
	UMaterial* CreateAdditiveUnlitTwoSidedMaterialWithParticleColor(const FString& ContentDir, const FString& MaterialName, UTexture2D* Texture);
	UTexture2D* ImportTexture(const FString& ImageFilePath, const FString& ContentDir);
	FString MakeUniqueContentDir(const FString& ContentDir, const FString& AssetName);

	UNiagaraNodeFunctionCall* AddEmitterModule(FVersionedNiagaraEmitterData* EmitterData, ENiagaraScriptUsage ScriptUsage, const FString& ModuleAssetPath);
	bool InsertNodeBefore(UEdGraphNode* BeforeNode, const FString::ElementType* InsertPinName, UEdGraphNode* TargetNode, const FString::ElementType* FrontPinName, const FString::ElementType* BackPinName);
	UNiagaraNodeFunctionCall* AddInputTransModule(FVersionedNiagaraEmitterData* EmitterData, UNiagaraNodeFunctionCall* TargetModule, const FString::ElementType* ParamName, const FNiagaraTypeDefinition& ParamType, const FString& ModuleAssetPath, const FString& ModuleOutPinName);
	UNiagaraNodeInput* AddInputNode(FVersionedNiagaraEmitterData* EmitterData, UNiagaraNodeFunctionCall* TargetModule, const FString::ElementType* ParamName, const FNiagaraTypeDefinition& ParamType, UNiagaraDataInterface** DefaultDataInterface);
	UNiagaraNodeFunctionCall* AddSubUVAnimationModule(FVersionedNiagaraEmitterData* EmitterData, UNiagaraSpriteRendererProperties* SpriteRenderer);

	bool SetEmitterModulePin(UNiagaraNodeFunctionCall* Node, const FString& PinName, const FString& Value);
	bool SetEmitterModulePinWithEnum(UNiagaraNodeFunctionCall* Node, const FString& PinName, const FString& EnumName, int64 Value);
	bool HaveValue(const FUPSSerializableCurve& Value);
	bool SetupModuleFloatCurveParam(FVersionedNiagaraEmitterData* EmitterData, UNiagaraNodeFunctionCall* Module, const FString::ElementType* ParamName, FUPSSerializableCurve& UnityCurve);
	bool SetupModuleSpeedCurveParam(FVersionedNiagaraEmitterData* EmitterData, UNiagaraNodeFunctionCall* Module, const FString::ElementType* ParamName, FUPSSerializableCurve& UnityCurve);
	bool SetupScaleColorModule(FVersionedNiagaraEmitterData* EmitterData, UNiagaraNodeFunctionCall* Module, FUPSSerializableMinMaxGradient& UnityGradient);
	bool SetupScaleSpriteSizeModule(FVersionedNiagaraEmitterData* EmitterData, UNiagaraNodeFunctionCall* Module, const FUPSSerializableCurve& Value);

	template<typename T>
	bool SetScriptParamValue(UNiagaraScript* Script, const FString::ElementType* ParamName, const T& Value)
	{
		FNiagaraVariable var(FNiagaraTypeDefinition::Get<T>(), ParamName);
		bool ret = Script->RapidIterationParameters.SetParameterValue<T>(Value, var);
		if (!ret)
		{
			UE_LOG(AnimationTools, Warning, TEXT("set param '%s' failed on script:%s"), ParamName, *Script->GetName());
		}
		return ret;
	}
	void SetScriptFloatParamUnityCurve(UNiagaraScript* Script, const FString::ElementType* EmitterName, const FString::ElementType* ParamName, const FUPSSerializableCurve& Value, float Scale = 1.0f, const FString::ElementType* ModuleName = nullptr);
	void SetScriptIntParamUnityCurve(UNiagaraScript* Script, const FString::ElementType* EmitterName, const FString::ElementType* ParamName, const FUPSSerializableCurve& Value, float Scale = 1.0f, const FString::ElementType* ModuleName = nullptr);
	void SetScriptSpeedParamUnityCurve(UNiagaraScript* Script, const FString::ElementType* EmitterName, const FString::ElementType* ParamName, const FUPSSerializableCurve& Value, float Scale = 1.0f, const FQuat4f& Rotation = FQuat4f(), const FString::ElementType* ModuleName = nullptr);
	void SetScaleColorUnityGradient(UNiagaraScript* Script, const FString::ElementType* EmitterName, const FUPSSerializableMinMaxGradient& Value);
	void SetScaleSpriteSizeCurve(UNiagaraScript* Script, const FString::ElementType* EmitterName, const FUPSSerializableCurve& Value);

	static void DumpGraph(TObjectPtr<UNiagaraGraph> Graph, FString Space, int Deep);
	static void DumpNode(TObjectPtr<UEdGraphNode> Node, FString Space);
	static void DumpPin(UEdGraphPin* Pin, FString Space);
	static void DumpScript(UNiagaraScript* Script, FString Space);

	static void OpenNiagaraScriptGraphViewer(UNiagaraGraph* NiagaraGraph, const FString::ElementType* GraphName);
};
