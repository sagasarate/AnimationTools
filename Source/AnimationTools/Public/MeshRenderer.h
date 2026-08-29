#pragma once
#include "CoreMinimal.h"
#include "PreviewScene.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/StreamableManager.h"

class USkeletalMeshComponent;

class FMeshRenderer
{
public:
	enum class ERenderState
	{
		None,
		MeshLoading,
		Rendering,
		RenderWaiting,
		Completed,
		Error,
	};

protected:
	// 纹理流送等待上限（帧）：已注册但未驻留的纹理最多等这么多帧
	// 连续批量渲染时前序任务的纹理仍占流送带宽，大贴图（如头发图集）排队最慢，故给足余量
	const int32 MaxStreamWaitTicks = 240;
	// 未注册纹理的宽限帧数：超过仍不注册即视为永久不可流送（占位小图），跳过不再等
	const int32						 UnregisteredGraceTicks = 60;
	ERenderState					 m_RenderState = ERenderState::None;
	int32							 m_RenderTickCount = 0;
	FPreviewScene					 m_Preview;
	TObjectPtr<UStaticMeshComponent> m_MeshComponent;
	// 骨骼网格体组件：与静态组件并存，渲染时按资产类型互斥显示
	TObjectPtr<USkeletalMeshComponent>	 m_SkeletalMeshComponent;
	TObjectPtr<USceneCaptureComponent2D> m_CaptureComponent;
	TObjectPtr<UTextureRenderTarget2D>	 m_RenderTarget;
	TArray<UTexture*>					 m_UsedTextures;
	// 当前渲染的网格体对象（流送句柄保活），供等待期间周期性重收纹理
	TWeakObjectPtr<UObject> m_CurrentMeshObj;
	// 静态/骨骼网格体通用，按实际类型 Cast 分流
	TSoftObjectPtr<UObject>		  m_SoftMeshPtr;
	TSharedPtr<FStreamableHandle> m_CurrentLoadHandle;
	int32						  m_OutSize = 128;
	int32						  m_AAMultiplier = 4; // 超采样倍率，4x = 16倍像素数
	float						  m_MeshRotation = 0.0f;
	float						  m_ViewRotation = 45.0f;
	float						  m_ViewAngle = 45.0f;
	float						  m_FOV = 35.0f;
	FImage						  m_OutputImage;
	UObject*					  m_Param;

public:
	FMeshRenderer();
	~FMeshRenderer();

	bool Render(const TSoftObjectPtr<UObject>& Mesh, int32 IconSize, float MeshRotationDeg, float ViewRotationDeg, float ViewAngleDeg, float FOVDeg);

	ERenderState				   TickRender(float InDeltaTime);
	const FImage&				   GetOutputImage() const { return m_OutputImage; }
	FString						   GetSourceName() const { return m_SoftMeshPtr.GetAssetName(); }
	const TSoftObjectPtr<UObject>& GetSoftMeshPtr() { return m_SoftMeshPtr; }
	void						   SetParam(UObject* InParam) { m_Param = InParam; }
	UObject*					   GetParam() const { return m_Param; }
	void						   Reset();

protected:
	void DoRender(UObject* LoadedMesh);
	// 重新收集当前网格体引用的全部纹理并补打 force 驻留（幂等）。
	// 材质的 expression 资源是延迟构建的，加载完成瞬间收集会残缺；
	// 等待期间周期性重收，清单随资源就绪而增长，漏收自愈
	void RecollectUsedTextures();
	void CheckRenderCompleted();
	void OnMeshLoaded(TSoftObjectPtr<UObject> InSoftMesh);
	void DoRenderCapture();
};