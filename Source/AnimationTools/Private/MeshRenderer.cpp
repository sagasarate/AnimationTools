// MeshRenderer.cpp
#include "MeshRenderer.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Brushes/SlateDynamicImageBrush.h"
#include "Engine/Texture2D.h"
#include "Styling/CoreStyle.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Modules/ModuleManager.h"
#include "SceneUtils.h" // 包含 FeatureLevel 相关定义
#include "TextureResource.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Engine/TextureCube.h"
#include "Misc/MessageDialog.h"
#include "ShaderCompiler.h"
#include "Engine/AssetManager.h"

// Box Filter 降采样（2x → 目标尺寸）
static void Downsample(FImage& Image, int32 Multiplier)
{
	const int32 SrcW = Image.SizeX;
	const int32 SrcH = Image.SizeY;
	const int32 DstW = SrcW / Multiplier;
	const int32 DstH = SrcH / Multiplier;
	const int32 Divisor = Multiplier * Multiplier;

	TArray<FColor> DstPixels;
	DstPixels.SetNumUninitialized(DstW * DstH);

	for (int32 Y = 0; Y < DstH; ++Y)
	{
		for (int32 X = 0; X < DstW; ++X)
		{
			int32 SumR = 0, SumG = 0, SumB = 0, SumA = 0;

			for (int32 DY = 0; DY < Multiplier; ++DY)
			{
				for (int32 DX = 0; DX < Multiplier; ++DX)
				{
					const FColor& SrcPixel = Image.AsBGRA8()[(Y * Multiplier + DY) * SrcW + (X * Multiplier + DX)];
					SumR += SrcPixel.R;
					SumG += SrcPixel.G;
					SumB += SrcPixel.B;
					SumA += SrcPixel.A;
				}
			}

			FColor& DstPixel = DstPixels[Y * DstW + X];
			DstPixel.R = static_cast<uint8>(SumR / Divisor);
			DstPixel.G = static_cast<uint8>(SumG / Divisor);
			DstPixel.B = static_cast<uint8>(SumB / Divisor);
			DstPixel.A = static_cast<uint8>(SumA / Divisor);
		}
	}

	Image.SizeX = DstW;
	Image.SizeY = DstH;
	Image.RawData.SetNumUninitialized(DstW * DstH * 4);
	FMemory::Memcpy(Image.RawData.GetData(), DstPixels.GetData(), DstW * DstH * 4);
}

FMeshRenderer::FMeshRenderer()
	: m_Preview(FPreviewScene::ConstructionValues().SetCreateDefaultLighting(true))
{
	m_Preview.SetLightBrightness(3.0f);
	m_Preview.SetLightDirection(FRotator(-45.0f, 45.0f, 0.0f));
	if (m_Preview.SkyLight)
	{
		UTextureCube* DefaultHDR = LoadObject<UTextureCube>(nullptr, TEXT("/AnimationTools/DaylightAmbientCubemap.DaylightAmbientCubemap"));
		m_Preview.SkyLight->SourceType = SLS_SpecifiedCubemap;
		m_Preview.SkyLight->Cubemap = DefaultHDR;
		m_Preview.SkyLight->Intensity = 1.5f;
		m_Preview.SkyLight->MarkRenderStateDirty();
	}

	m_MeshComponent = NewObject<UStaticMeshComponent>();
	m_MeshComponent->SetMobility(EComponentMobility::Movable);
	m_MeshComponent->SetRenderCustomDepth(true);
	m_MeshComponent->SetCustomDepthStencilValue(1);
	m_MeshComponent->UpdateComponentToWorld();
	m_Preview.AddComponent(m_MeshComponent, FTransform::Identity);

	UMaterial* StencilPP = LoadObject<UMaterial>(nullptr, TEXT("/AnimationTools/M_TransparentBack.M_TransparentBack"));
	StencilPP->ForceRecompileForRendering();

	m_CaptureComponent = NewObject<USceneCaptureComponent2D>();
	m_CaptureComponent->ProjectionType = ECameraProjectionMode::Perspective;
	m_CaptureComponent->CaptureSource = SCS_FinalColorLDR;
	m_CaptureComponent->bCaptureEveryFrame = false;
	m_CaptureComponent->bCaptureOnMovement = false;
	m_CaptureComponent->bAlwaysPersistRenderingState = true;
	m_CaptureComponent->PostProcessSettings.WeightedBlendables.Array.Empty();
	m_CaptureComponent->PostProcessSettings.AddBlendable(StencilPP, 1.0f);
	m_CaptureComponent->PostProcessBlendWeight = 1.0f;
	m_Preview.AddComponent(m_CaptureComponent, FTransform::Identity);

	m_Preview.UpdateCaptureContents();
}

FMeshRenderer::~FMeshRenderer()
{
	if (m_CurrentLoadHandle.IsValid())
	{
		m_CurrentLoadHandle->CancelHandle();
	}
}

bool FMeshRenderer::Render(const TSoftObjectPtr<UStaticMesh>& Mesh, int32 IconSize, float MeshRotationDeg, float ViewRotationDeg, float ViewAngleDeg, float FOVDeg)
{
	m_SoftMeshPtr = Mesh;
	UE_LOG(LogTemp, Log, TEXT("Start Render for mesh: %s"), *m_SoftMeshPtr.ToString());
	m_RenderState = ERenderState::MeshLoading;
	m_OutSize = IconSize;
	m_MeshRotation = MeshRotationDeg;
	m_ViewRotation = ViewRotationDeg;
	m_ViewAngle = ViewAngleDeg;
	m_FOV = FOVDeg;
	m_RenderTarget = NewObject<UTextureRenderTarget2D>();
	m_RenderTarget->RenderTargetFormat = RTF_RGBA8;
	m_RenderTarget->ClearColor = FLinearColor::Transparent;
	m_RenderTarget->bForceLinearGamma = false; // 确保使用 sRGB 转换
	m_RenderTarget->TargetGamma = 2.2f;		   // 显式指定 Gamma 值
	m_RenderTarget->InitAutoFormat(m_OutSize * m_AAMultiplier, m_OutSize * m_AAMultiplier);
	m_RenderTarget->bAutoGenerateMips = false;
	m_RenderTarget->UpdateResourceImmediate(true);
	m_CaptureComponent->TextureTarget = m_RenderTarget;
	m_Preview.UpdateCaptureContents();
	FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
	if (m_CurrentLoadHandle.IsValid())
	{
		m_CurrentLoadHandle->CancelHandle();
	}
	m_CurrentLoadHandle = Streamable.RequestAsyncLoad(
		m_SoftMeshPtr.ToSoftObjectPath(),
		FStreamableDelegate::CreateRaw(this, &FMeshRenderer::OnMeshLoaded, m_SoftMeshPtr));
	return true;
}

FMeshRenderer::ERenderState FMeshRenderer::TickRender(float InDeltaTime)
{
	IStreamingManager::Get().Tick(InDeltaTime);
	m_Preview.GetWorld()->Tick(LEVELTICK_All, InDeltaTime);
	m_RenderTickCount++;
	switch (m_RenderState)
	{
		case ERenderState::Rendering:
			CheckRenderCompleted();
			break;
		case ERenderState::RenderWaiting:
			DoRenderCapture();
			break;
	}
	return m_RenderState;
}

void FMeshRenderer::DoRender(UStaticMesh* Mesh)
{
	UE_LOG(LogTemp, Log, TEXT("DoRender for mesh: %s"), *m_SoftMeshPtr.ToString());
	if (!Mesh)
	{
		m_RenderState = ERenderState::Error;
		return;
	}

	m_MeshComponent->SetStaticMesh(Mesh);
	m_MeshComponent->SetRelativeRotation(FRotator(0.f, m_MeshRotation, 0.f));
	m_MeshComponent->UpdateComponentToWorld();
	m_MeshComponent->UpdateBounds();

	m_CaptureComponent->FOVAngle = m_FOV;

	// Compute geometry: bounding sphere
	FBoxSphereBounds Bounds = m_MeshComponent->Bounds;
	FVector			 Center = Bounds.Origin;
	float			 Radius = Bounds.SphereRadius;

	if (Radius <= KINDA_SMALL_NUMBER)
	{
		m_RenderState = ERenderState::Error;
		return;
	}

	// Target point: lowest point on bounding sphere
	FVector Target = Center;

	// Prepare angles
	float Alpha = FMath::DegreesToRadians(m_ViewRotation);
	float Beta = FMath::DegreesToRadians(m_ViewAngle);

	// Direction from camera to target (unit) based on alpha/beta
	// D points from camera to target
	FVector D = FVector(FMath::Cos(Beta) * FMath::Cos(Alpha), FMath::Cos(Beta) * FMath::Sin(Alpha), -FMath::Sin(Beta));
	D = D.GetSafeNormal();

	// We'll find a distance along -D such that the bounding sphere fully fits in the camera FOV
	float HalfFOV = FMath::DegreesToRadians(m_FOV * 0.5f);
	float Distance = Radius * 2.5f + 10.0f;
	int	  Iter = 0;
	for (; Iter < 64; ++Iter)
	{
		FVector CamPos = Target - D * Distance; // camera position
		float	DistToCenter = (Center - CamPos).Size();
		if (DistToCenter <= KINDA_SMALL_NUMBER)
		{
			Distance *= 1.5f;
			continue;
		}

		float theta_c = FMath::Acos(FVector::DotProduct((Center - CamPos).GetSafeNormal(), D));
		float phi = FMath::Asin(FMath::Min(Radius / DistToCenter, 0.9999f));
		if (theta_c + phi <= HalfFOV)
		{
			break; // fits
		}
		Distance *= 1.1f; // increase distance and try again
	}

	FVector	 CamLocation = Target - D * Distance;
	FRotator CamRot = (Target - CamLocation).Rotation();
	m_CaptureComponent->SetWorldLocationAndRotation(CamLocation, CamRot);

	m_UsedTextures.Empty();
	for (const FStaticMaterial& StaticMat : Mesh->GetStaticMaterials())
	{
		if (StaticMat.MaterialInterface)
		{
			StaticMat.MaterialInterface->GetUsedTextures(
				m_UsedTextures,
				EMaterialQualityLevel::High);
		}
	}

	for (UTexture* Tex : m_UsedTextures)
	{
		if (UTexture2D* Tex2D = Cast<UTexture2D>(Tex))
		{
			Tex2D->SetForceMipLevelsToBeResident(30.0f);
		}
	}
	IStreamingManager::Get().NotifyPrimitiveUpdated(m_MeshComponent);

	m_RenderState = ERenderState::Rendering;
}

void FMeshRenderer::CheckRenderCompleted()
{
	for (UTexture* Tex : m_UsedTextures)
	{
		if (UTexture2D* Tex2D = Cast<UTexture2D>(Tex))
		{
			if (Tex2D->IsStreamable())
			{
				if (!Tex2D->IsFullyStreamedIn())
					return;
			}
		}
	}
	if (GShaderCompilingManager && GShaderCompilingManager->IsCompiling())
	{
		return; // 继续等待，Shader 还没好
	}

	m_RenderState = ERenderState::RenderWaiting;
	m_RenderTickCount = 0;

	UE_LOG(LogTemp, Log, TEXT("Render Wait for mesh: %s"), *m_SoftMeshPtr.ToString());
}

void FMeshRenderer::OnMeshLoaded(TSoftObjectPtr<UStaticMesh> InSoftMesh)
{
	UE_LOG(LogTemp, Log, TEXT("finish load for mesh: %s"), *m_SoftMeshPtr.ToString());
	UStaticMesh* LoadedMesh = InSoftMesh.Get();
	if (LoadedMesh)
	{
		DoRender(LoadedMesh);
	}
	else
	{
		m_RenderState = ERenderState::Error;
	}
	m_CurrentLoadHandle.Reset();
}

void FMeshRenderer::DoRenderCapture()
{
	FlushRenderingCommands();
	UE_LOG(LogTemp, Log, TEXT("DoRenderCapture for mesh: %s"), *m_SoftMeshPtr.ToString());
	m_CaptureComponent->CaptureScene();
	if (!FImageUtils::GetRenderTargetImage(m_RenderTarget, m_OutputImage))
	{
		m_RenderState = ERenderState::Error;
		return;
	}
	if (m_OutputImage.SizeX <= 0 || m_OutputImage.SizeY <= 0)
	{
		m_RenderState = ERenderState::Error;
		return;
	}

	// 纯超采样抗锯齿：Box Filter降采样
	Downsample(m_OutputImage, m_AAMultiplier);

	m_RenderState = ERenderState::Completed;
}

void FMeshRenderer::Reset()
{
	if (m_CurrentLoadHandle.IsValid())
	{
		m_CurrentLoadHandle->CancelHandle();
		m_CurrentLoadHandle.Reset();
	}
	m_RenderState = ERenderState::None;
}