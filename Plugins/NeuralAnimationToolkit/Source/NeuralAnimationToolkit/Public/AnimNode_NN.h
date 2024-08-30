#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/InputScaleBias.h"
#include "AnimationRuntime.h"
#include "Animation/AnimInstanceProxy.h"
#include "BoneContainer.h"
#include "NNE.h"
#include "NNERuntimeCPU.h"
#include "NNEModelData.h"
#include "ModelInstance.h"
#include "Features.h"
#include "Springs.h"
#include "AnimNode_NN.generated.h"


USTRUCT(BlueprintInternalUseOnly)
struct NEURALANIMATIONTOOLKIT_API FAnimNode_NN : public FAnimNode_Base
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Settings)
	FPoseLink Source;

	UPROPERTY(EditAnywhere, Category = Settings)
	TObjectPtr<UFeatureSet> FeatureSet;

	UPROPERTY(EditAnywhere, Category = Settings)
	TObjectPtr<UNNEModelData> ModelData;

	UPROPERTY(EditAnywhere, Category = Settings, meta = (PinShownByDefault))
	bool isRunning;

	UPROPERTY(EditAnywhere, Category = Settings, meta = (PinShownByDefault))
	bool isAsync = false;

	UPROPERTY(EditAnywhere, Category = Settings)
	bool isInertialised = false;

	UPROPERTY(EditAnywhere, Category = Settings)
	float halfLife = 0.5f;

	FAnimNode_NN();

public:
	// FAnimNode_Base interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface

private:
	TSharedPtr<FModelInstance> ModelInstance;
	bool isModelInitialized = false;
	bool isBonesRefInitialized = false;
	TArray<FVector> BonePositions;
	TArray<FQuat> BoneRotations;
	TArray<FVector> BoneVelocities;
	TArray<FVector> BoneAngularVelocities;
	TArray<FTransformSpring> Inertializers;

	void SetLocalBoneTransforms(FPoseContext& Output, const FBoneContainer& BoneContainer);
	void SetComponentSpaceBoneTransforms(FPoseContext& Output, const FBoneContainer& BoneContainer);
	void InitializeModel(TObjectPtr<UNNEModelData> modelData);
	int EvaluateModel(TArray<float>& InputData, const float DeltaTime);
	int ProcessOutput(const float DeltaTime);
};