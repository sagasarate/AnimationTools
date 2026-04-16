#pragma once

#include "CoreMinimal.h"
#include "Kismet/KismetMathLibrary.h"
#include "UnityParticleSystem.generated.h"

USTRUCT()
struct FUPSKeyFrame
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	float time = 0;
	UPROPERTY()
	float value = 0;
	UPROPERTY()
	float inTangent = 0;
	UPROPERTY()
	float outTangent = 0;
	UPROPERTY()
	float inWeight = 0;
	UPROPERTY()
	float outWeight = 0;
	UPROPERTY()
	int weightedMode = 0;
	UPROPERTY()
	int tangentMode = 0;
	void RadiansToDegrees()
	{
		value = UKismetMathLibrary::RadiansToDegrees(value);
	}
	void Rotate(float Rotation)
	{
		value += Rotation;
	}
};

USTRUCT()
struct FUPSAnimationCurve
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	TArray<FUPSKeyFrame> keys;
	UPROPERTY()
	int length = 0;
	UPROPERTY()
	int preWrapMode = 0;
	UPROPERTY()
	int postWrapMode = 0;
	void RadiansToDegrees()
	{
		for (FUPSKeyFrame &key : keys)
			key.RadiansToDegrees();
	}
	void Rotate(float Rotation)
	{
		for (FUPSKeyFrame &key : keys)
			key.Rotate(Rotation);
	}
};

USTRUCT()
struct FUPSSerializableCurve
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	FString mode;
	UPROPERTY()
	float constantMin = 0;
	UPROPERTY()
	float constantMax = 0;
	UPROPERTY()
	FUPSAnimationCurve curveMin;
	UPROPERTY()
	FUPSAnimationCurve curveMax;

	FString BindModuleName;
	void RadiansToDegrees()
	{
		constantMin = UKismetMathLibrary::RadiansToDegrees(constantMin);
		constantMax = UKismetMathLibrary::RadiansToDegrees(constantMax);
		curveMin.RadiansToDegrees();
		curveMax.RadiansToDegrees();
	}
	void Rotate(float Rotation)
	{
		constantMin += Rotation;
		constantMax += Rotation;
		curveMin.Rotate(Rotation);
		curveMax.Rotate(Rotation);
	}
};

USTRUCT()
struct FUPSSerializableGradientColorKey
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	float time = 0;
	UPROPERTY()
	FString color;
};

USTRUCT()
struct FUPSSerializableGradientAlphaKey
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	float time = 0;
	UPROPERTY()
	float alpha = 0;
};

USTRUCT()
struct FUPSSerializableGradient
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	FString mode;
	UPROPERTY()
	TArray<FUPSSerializableGradientColorKey> colorKeys;
	UPROPERTY()
	TArray<FUPSSerializableGradientAlphaKey> alphaKeys;
};

USTRUCT()
struct FUPSSerializableMinMaxGradient
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	FString mode;
	UPROPERTY()
	FString colorMin;
	UPROPERTY()
	FString colorMax;
	UPROPERTY()
	FUPSSerializableGradient gradientMin;
	UPROPERTY()
	FUPSSerializableGradient gradientMax;

	FString BindModuleName;
};

USTRUCT()
struct FUPSBurstData
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	float time = 0;
	UPROPERTY()
	FUPSSerializableCurve count;
	UPROPERTY()
	int16 minCount = 0;
	UPROPERTY()
	int16 maxCount = 0;
	UPROPERTY()
	int cycleCount = 0;
	UPROPERTY()
	float repeatInterval = 0;
	UPROPERTY()
	float probability = 0;

	TArray<FString> BindModuleNames;
};

USTRUCT()
struct FUPSTransformData
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	FString name;
    UPROPERTY()
    FVector3f position = FVector3f::ZeroVector;
	UPROPERTY()
	FQuat4d rotation = FQuat4d::Identity;
	UPROPERTY()
	FVector3f scale = FVector3f::OneVector;
};

USTRUCT()
struct FUPSMaterialData
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	FString color;
	UPROPERTY()
	FString mainTexture;
	UPROPERTY()
	FString shader;
};

USTRUCT()
struct FUPSMainModuleData
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	float duration = 0;
	UPROPERTY()
	bool loop = false;
	UPROPERTY()
	bool prewarm = false;
	UPROPERTY()
	FUPSSerializableCurve startDelay;
	UPROPERTY()
	FUPSSerializableCurve startLifetime;
	UPROPERTY()
	FUPSSerializableCurve startSpeed;
	UPROPERTY()
	bool startSize3D = 0;
	UPROPERTY()
	FUPSSerializableCurve startSize;
	UPROPERTY()
	bool startRotation3D = 0;
	UPROPERTY()
	FUPSSerializableCurve startRotation;
	UPROPERTY()
	float flipRotation = 0;
	UPROPERTY()
	FUPSSerializableMinMaxGradient startColor;
	UPROPERTY()
	FUPSSerializableCurve gravityModifier;
	UPROPERTY()
	FString simulationSpace;
	UPROPERTY()
	FUPSTransformData customSimulationSpace;
	UPROPERTY()
	float simulationSpeed = 0;
	UPROPERTY()
	bool useUnscaledTime = 0;
	UPROPERTY()
	FString scalingMode;
	UPROPERTY()
	bool playOnAwake = 0;
	UPROPERTY()
	FString emitterVelocityMode;
	UPROPERTY()
	int maxParticles = 0;
	UPROPERTY()
	FString stopAction;
	UPROPERTY()
	FString cullingMode;
	UPROPERTY()
	FString ringBufferMode;
	UPROPERTY()
	FVector2D ringBufferLoopRange = FVector2D::ZeroVector;
};

USTRUCT()
struct FUPSEmissionModuleData
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	bool enable = false;
	UPROPERTY()
	FUPSSerializableCurve rateOverTime;
	UPROPERTY()
	FUPSSerializableCurve rateOverDistance;
	UPROPERTY()
	TArray<FUPSBurstData> bursts;
};

USTRUCT()
struct FUPSShapeModuleData
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	bool enable = false;
	UPROPERTY()
	FString shapeType;
	UPROPERTY()
	float radius = 0;
	UPROPERTY()
	float radiusThickness = 0;
	UPROPERTY()
	float arc = 0;
	UPROPERTY()
	FString arcMode;
	UPROPERTY()
	float arcSpread = 0;
	UPROPERTY()
	FUPSSerializableCurve arcSpeed;
	UPROPERTY()
	FString texture;
	UPROPERTY()
	FVector3f position = FVector3f::ZeroVector;
	UPROPERTY()
	FVector3f rotation = FVector3f::ZeroVector;
	UPROPERTY()
	FVector3f scale = FVector3f::OneVector;
	UPROPERTY()
	bool alignToDirection = false;
	UPROPERTY()
	float randomDirectionAmount = 0;
	UPROPERTY()
	float sphericalDirectionAmount = 0;
	UPROPERTY()
	float randomPositionAmount = 0;
	UPROPERTY()
	float angle = 0;
	UPROPERTY()
	float length = 0;
	UPROPERTY()
	FString mesh;
	UPROPERTY()
	FString meshShapeType;
	UPROPERTY()
	bool useMeshMaterialIndex = false;
	UPROPERTY()
	int meshMaterialIndex = 0;
	UPROPERTY()
	float normalOffset = 0;
	UPROPERTY()
	FString sprite;
};

USTRUCT()
struct FUPSVelocityOverLifetimeModuleData
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	bool enable = false;
	UPROPERTY()
	FUPSSerializableCurve x;
	UPROPERTY()
	FUPSSerializableCurve y;
	UPROPERTY()
	FUPSSerializableCurve z;
	UPROPERTY()
	FString space;
	UPROPERTY()
	FUPSSerializableCurve orbitalX;
	UPROPERTY()
	FUPSSerializableCurve orbitalY;
	UPROPERTY()
	FUPSSerializableCurve orbitalZ;
	UPROPERTY()
	FUPSSerializableCurve radial;
	UPROPERTY()
	FUPSSerializableCurve speedModifier;
};

USTRUCT()
struct FUPSLimitVelocityOverLifetimeModuleData
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	bool enable = false;
	UPROPERTY()
	bool separateAxes = false;
	UPROPERTY()
	FUPSSerializableCurve limitX;
	UPROPERTY()
	FUPSSerializableCurve limitY;
	UPROPERTY()
	FUPSSerializableCurve limitZ;
	UPROPERTY()
	FString space;
	UPROPERTY()
	float dampen = 0;
	UPROPERTY()
	FUPSSerializableCurve drag;
	UPROPERTY()
	bool multiplyDragByParticleSize = false;
	UPROPERTY()
	bool multiplyDragByParticleVelocity = false;
};

USTRUCT()
struct FUPSInheritVelocityModuleData
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	bool enable = false;
	UPROPERTY()
	FString mode;
	UPROPERTY()
	FUPSSerializableCurve curve;
};

USTRUCT()
struct FUPSForceOverLifetimeModuleData
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	bool enable = false;
	UPROPERTY()
	FUPSSerializableCurve x;
	UPROPERTY()
	FUPSSerializableCurve y;
	UPROPERTY()
	FUPSSerializableCurve z;
	UPROPERTY()
	FString space;
	UPROPERTY()
	bool randomized = false;
};

USTRUCT()
struct FUPSColorOverLifetimeModuleData
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	bool enable = false;
	UPROPERTY()
	FUPSSerializableMinMaxGradient color;
};

USTRUCT()
struct FUPSColorBySpeedModuleData
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	bool enable = false;
	UPROPERTY()
	FUPSSerializableMinMaxGradient color;
	UPROPERTY()
	FVector2D range = FVector2D::ZeroVector;
};

USTRUCT()
struct FUPSSizeOverLifetimeModuleData
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	bool enable = false;
	UPROPERTY()
	FUPSSerializableCurve size;
	UPROPERTY()
	bool separateAxes = false;
};

USTRUCT()
struct FUPSSizeBySpeedModuleData
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	bool enable = false;
	UPROPERTY()
	FUPSSerializableCurve size;
	UPROPERTY()
	bool separateAxes = false;
	UPROPERTY()
	FVector2D range = FVector2D::ZeroVector;
};

USTRUCT()
struct FUPSRotationOverLifetimeModuleData
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	bool enable = false;
	UPROPERTY()
	FUPSSerializableCurve x;
	UPROPERTY()
	FUPSSerializableCurve y;
	UPROPERTY()
	FUPSSerializableCurve z;
	UPROPERTY()
	bool separateAxes = false;
};

USTRUCT()
struct FUPSRotationBySpeedModuleData
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	bool enable = false;
	UPROPERTY()
	FUPSSerializableCurve x;
	UPROPERTY()
	FUPSSerializableCurve y;
	UPROPERTY()
	FUPSSerializableCurve z;
	UPROPERTY()
	bool separateAxes = false;
	UPROPERTY()
	FVector2D range = FVector2D::ZeroVector;
};

USTRUCT()
struct FUPSExternalForcesModuleData
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	bool enable = false;
	UPROPERTY()
	FUPSSerializableCurve multiplierCurve;
	UPROPERTY()
	FString influenceFilter;
	UPROPERTY()
	int influenceMask = false;
};

USTRUCT()
struct FUPSNoiseModuleData
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	bool enable = false;
	UPROPERTY()
	bool separateAxes = false;
	UPROPERTY()
	FUPSSerializableCurve strengthX;
	UPROPERTY()
	FUPSSerializableCurve strengthY;
	UPROPERTY()
	FUPSSerializableCurve strengthZ;
	UPROPERTY()
	float frequency = 0;
	UPROPERTY()
	FUPSSerializableCurve scrollSpeed;
	UPROPERTY()
	bool damping = false;
	UPROPERTY()
	int octaveCount = 0;
	UPROPERTY()
	float octaveMultiplier = 0;
	UPROPERTY()
	float octaveScale = 0;
	UPROPERTY()
	FString quality;
	UPROPERTY()
	bool remapEnabled = false;
	UPROPERTY()
	FUPSSerializableCurve remap;
	UPROPERTY()
	FUPSSerializableCurve positionAmount;
	UPROPERTY()
	FUPSSerializableCurve rotationAmount;
	UPROPERTY()
	FUPSSerializableCurve sizeAmount;
};

USTRUCT()
struct FUPSCollisionModuleData
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	bool enable = false;
	UPROPERTY()
	FString type;
	UPROPERTY()
	TArray<FUPSTransformData> planes;
	UPROPERTY()
	FUPSSerializableCurve dampen;
	UPROPERTY()
	FUPSSerializableCurve bounce;
	UPROPERTY()
	FUPSSerializableCurve lifetimeLoss;
	UPROPERTY()
	float minKillSpeed = 0;
	UPROPERTY()
	float maxKillSpeed = 0;
	UPROPERTY()
	float radiusScale = 0;
	UPROPERTY()
	bool sendCollisionMessages = false;
	UPROPERTY()
	FString mode;
	UPROPERTY()
	FString quality;
	UPROPERTY()
	int collidesWith = 0;
	UPROPERTY()
	int maxCollisionShapes = 0;
	UPROPERTY()
	bool enableDynamicColliders = false;
	UPROPERTY()
	float voxelSize = 0;
	UPROPERTY()
	float colliderForce = 0;
	UPROPERTY()
	bool multiplyColliderForceByCollisionAngle = false;
	UPROPERTY()
	bool multiplyColliderForceByParticleSpeed = false;
	UPROPERTY()
	bool multiplyColliderForceByParticleSize = false;
};

USTRUCT()
struct FUPSTriggerModuleData
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	bool enable = false;
	UPROPERTY()
	TArray<FString> Colliders;
	UPROPERTY()
	FString inside;
	UPROPERTY()
	FString outside;
	UPROPERTY()
	FString enter;
	UPROPERTY()
	FString exit;
	UPROPERTY()
	float radiusScale = 0;
};

// struct FUPSParticleSystemData;
//
// USTRUCT()
// struct FUPSSubEmitterSystemInfo
//{
//	GENERATED_USTRUCT_BODY()
// public:
//	UPROPERTY()
//	FString type;
//	UPROPERTY()
//	FUPSParticleSystemData ParticleData;
//	UPROPERTY()
//	int Inherit;
// };
//
// USTRUCT()
// struct FUPSSubEmittersModuleData
//{
//	GENERATED_USTRUCT_BODY()
// public:
//	UPROPERTY()
//	TArray<FUPSSubEmitterSystemInfo> subEmitter;
// };

USTRUCT()
struct FUPSTextureSheetAnimationModuleData
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	bool enable = false;
	UPROPERTY()
	FString mode;
	UPROPERTY()
	int numTilesX = 0;
	UPROPERTY()
	int numTilesY = 0;
	UPROPERTY()
	FString animation;
	UPROPERTY()
	FString timeMode;
	UPROPERTY()
	FUPSSerializableCurve frameOverTime;
	UPROPERTY()
	FUPSSerializableCurve startFrame;
	UPROPERTY()
	int cycleCount = 0;
	UPROPERTY()
	int uvChannelMask = 0;
	UPROPERTY()
	TArray<FString> sprites;
};

USTRUCT()
struct FUPSLightsModuleData
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	bool enable = false;
	UPROPERTY()
	FString light;
	UPROPERTY()
	float ratio = 0;
	UPROPERTY()
	bool useRandomDistribution = false;
	UPROPERTY()
	bool useParticleColor = false;
	UPROPERTY()
	bool sizeAffectsRange = false;
	UPROPERTY()
	bool alphaAffectsIntensity = false;
	UPROPERTY()
	FUPSSerializableCurve range;
	UPROPERTY()
	FUPSSerializableCurve intensity;
	UPROPERTY()
	int maxLights = 0;
};

USTRUCT()
struct FUPSTrailModuleData
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	bool enable = false;
	UPROPERTY()
	FString mode;
	UPROPERTY()
	float ratio = 0;
	UPROPERTY()
	FUPSSerializableCurve lifetime;
	UPROPERTY()
	float minVertexDistance = 0;
	UPROPERTY()
	bool worldSpace = false;
	UPROPERTY()
	bool dieWithParticles = false;
	UPROPERTY()
	FString textureMode;
	UPROPERTY()
	bool sizeAffectsWidth = false;
	UPROPERTY()
	bool sizeAffectsLifetime = false;
	UPROPERTY()
	bool inheritParticleColor = false;
	UPROPERTY()
	FUPSSerializableMinMaxGradient colorOverLifetime;
	UPROPERTY()
	FUPSSerializableCurve widthOverTrail;
	UPROPERTY()
	FUPSSerializableMinMaxGradient colorOverTrail;
	UPROPERTY()
	bool generateLightingData = false;
	UPROPERTY()
	float shadowBias = 0;
	UPROPERTY()
	int ribbonCount = 0;
	UPROPERTY()
	bool splitSubEmitterRibbons = false;
	UPROPERTY()
	bool attachRibbonsToTransform = false;
};

USTRUCT()
struct FUPSCustomData
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	FString mode;
	UPROPERTY()
	int vectorComponentCount = 0;
	UPROPERTY()
	TArray<FUPSSerializableCurve> vector;
	UPROPERTY()
	FUPSSerializableMinMaxGradient color;
};

USTRUCT()
struct FUPSCustomDataModuleData
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	bool enable = false;
	UPROPERTY()
	FUPSCustomData CustomData1;
	UPROPERTY()
	FUPSCustomData CustomData2;
};

USTRUCT()
struct FUPSParticleSystemRendererData
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	bool enabled = false;
	UPROPERTY()
	FString renderMode;
	UPROPERTY()
	float normalDirection = 0;
	UPROPERTY()
	FUPSMaterialData material;
	UPROPERTY()
	FUPSMaterialData trailMaterial;
	UPROPERTY()
	FString sortMode;
	UPROPERTY()
	float sortingFudge = 0;
	UPROPERTY()
	float minParticleSize = 0;
	UPROPERTY()
	float maxParticleSize = 0;
	UPROPERTY()
	FString alignment;
	UPROPERTY()
	FVector3f flip = FVector3f::ZeroVector;
	UPROPERTY()
	bool allowRoll = false;
	UPROPERTY()
	FVector3f pivot = FVector3f::ZeroVector;
	UPROPERTY()
	FString shadowCastingMode;
	UPROPERTY()
	bool receiveShadows = false;
	UPROPERTY()
	float shadowBias = 0;
	UPROPERTY()
	FString motionVectorGenerationMode;
	UPROPERTY()
	FString sortingLayerName;
	UPROPERTY()
	int sortingOrder = 0;
	UPROPERTY()
	float lengthScale = 0;
	UPROPERTY()
	float velocityScale = 0;
	UPROPERTY()
	float cameraVelocityScale = 0;
	UPROPERTY()
	FString mesh;

	FString ScaleSpriteSizeBySpeedModuleName;
	FString SpriteFacingModuleName;
};

USTRUCT()
struct FUPSParticleSystemData
{
	GENERATED_USTRUCT_BODY()
public:
	bool beExport = true;
	FGuid EmitterHandleID;
	UPROPERTY()
	FString name;
	UPROPERTY()
	bool useAutoRandomSeed = false;
	UPROPERTY()
	uint16 randomSeed = 0;
	UPROPERTY()
	FVector3f position = FVector3f::ZeroVector;
	UPROPERTY()
	FVector3f rotation = FVector3f::ZeroVector;
	UPROPERTY()
	FVector3f scale = FVector3f::OneVector;
	UPROPERTY()
	FUPSMainModuleData main;
	UPROPERTY()
	FUPSEmissionModuleData emission;
	UPROPERTY()
	FUPSShapeModuleData shape;
	UPROPERTY()
	FUPSVelocityOverLifetimeModuleData velocityOverLifetime;
	UPROPERTY()
	FUPSLimitVelocityOverLifetimeModuleData limitVelocityOverLifetime;
	UPROPERTY()
	FUPSInheritVelocityModuleData inheritVelocity;
	UPROPERTY()
	FUPSForceOverLifetimeModuleData forceOverLifetime;
	UPROPERTY()
	FUPSColorOverLifetimeModuleData colorOverLifetime;
	UPROPERTY()
	FUPSColorBySpeedModuleData colorBySpeed;
	UPROPERTY()
	FUPSSizeOverLifetimeModuleData sizeOverLifetime;
	UPROPERTY()
	FUPSSizeBySpeedModuleData sizeBySpeed;
	UPROPERTY()
	FUPSRotationOverLifetimeModuleData rotationOverLifetime;
	UPROPERTY()
	FUPSRotationBySpeedModuleData rotationBySpeed;
	UPROPERTY()
	FUPSExternalForcesModuleData externalForces;
	UPROPERTY()
	FUPSNoiseModuleData noise;
	UPROPERTY()
	FUPSCollisionModuleData collision;
	UPROPERTY()
	FUPSTriggerModuleData trigger;
	// UPROPERTY()
	// FUPSSubEmittersModuleData subEmitters;
	UPROPERTY()
	FUPSTextureSheetAnimationModuleData textureSheetAnimation;
	UPROPERTY()
	FUPSLightsModuleData lights;
	UPROPERTY()
	FUPSTrailModuleData trails;
	UPROPERTY()
	FUPSCustomDataModuleData customData;
	UPROPERTY()
	FUPSParticleSystemRendererData rendererData;
};

inline FLinearColor ColorStr2LinearColor(const FString &ColorStr)
{
	return FLinearColor::FromSRGBColor(FColor::FromHex(ColorStr));
}

inline FVector3f UnityPosition2UE(const FVector3f &Value)
{
	return FVector3f(Value.Z, Value.X, Value.Y);
}

inline FVector3f UnityRotation2UE(const FVector3f &Value)
{
	return FVector3f(-Value.Z, -Value.X, Value.Y);
}
