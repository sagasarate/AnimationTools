#pragma once
#include "CoreMinimal.h"
#include "PreviewScene.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/StreamableManager.h"

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
	const int32							 RenderWaitTicks = 1;
	ERenderState						 m_RenderState = ERenderState::None;
	int32								 m_RenderTickCount = 0;
	FPreviewScene						 m_Preview;
	TObjectPtr<UStaticMeshComponent>	 m_MeshComponent;
	TObjectPtr<USceneCaptureComponent2D> m_CaptureComponent;
	TObjectPtr<UTextureRenderTarget2D>	 m_RenderTarget;
	TArray<UTexture*>					 m_UsedTextures;
	TSoftObjectPtr<UStaticMesh>			 m_SoftMeshPtr;
	TSharedPtr<FStreamableHandle>		 m_CurrentLoadHandle;
	int32								 m_OutSize = 128;
	int32								 m_AAMultiplier = 4; // 超采样倍率，4x = 16倍像素数
	float								 m_MeshRotation = 0.0f;
	float								 m_ViewRotation = 45.0f;
	float								 m_ViewAngle = 45.0f;
	float								 m_FOV = 35.0f;
	FImage								 m_OutputImage;
	UObject*							 m_Param;

public:
	FMeshRenderer();
	~FMeshRenderer();

	bool Render(const TSoftObjectPtr<UStaticMesh>& Mesh, int32 IconSize, float MeshRotationDeg, float ViewRotationDeg, float ViewAngleDeg, float FOVDeg);

	ERenderState					   TickRender(float InDeltaTime);
	const FImage&					   GetOutputImage() const { return m_OutputImage; }
	FString							   GetSourceName() const { return m_SoftMeshPtr.GetAssetName(); }
	const TSoftObjectPtr<UStaticMesh>& GetSoftMeshPtr() { return m_SoftMeshPtr; }
	void							   SetParam(UObject* InParam) { m_Param = InParam; }
	UObject*						   GetParam() const { return m_Param; }
	void							   Reset();

protected:
	void DoRender(UStaticMesh* Mesh);
	void CheckRenderCompleted();
	void OnMeshLoaded(TSoftObjectPtr<UStaticMesh> InSoftMesh);
	void DoRenderCapture();
};