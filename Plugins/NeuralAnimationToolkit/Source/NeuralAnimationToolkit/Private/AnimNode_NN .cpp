#include "AnimNode_NN.h"

FAnimNode_NN::FAnimNode_NN() {
}

void FAnimNode_NN::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Initialize_AnyThread)
	Super::Initialize_AnyThread(Context);
	Source.Initialize(Context);

	if (isInertialised) {
		for (FBoneReference& BoneReference : FeatureSet->OutputBones) {
			Inertializers.Add(FTransformSpring(halfLife));
		}
	}
}

void FAnimNode_NN::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(CacheBones_AnyThread)
	Source.CacheBones(Context);
	const FBoneContainer& BoneContainer = Context.AnimInstanceProxy->GetRequiredBones();
	for (FBoneReference& BoneReference : FeatureSet->OutputBones)
	{
		BoneReference.Initialize(BoneContainer);
	}
	FeatureSet->InitialiseFeaturesRealTime(BoneContainer);
}

void FAnimNode_NN::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Update_AnyThread)
	GetEvaluateGraphExposedInputs().Execute(Context);
	Source.Update(Context);

	if (FeatureSet->OutputBones.Num() != 0 && !isBonesRefInitialized) {
		const FBoneContainer& BoneContainer = Context.AnimInstanceProxy->GetRequiredBones();
		for (FBoneReference& BoneReference : FeatureSet->OutputBones) {
			BoneReference.Initialize(BoneContainer);
		}
		FeatureSet->InitialiseFeaturesRealTime(BoneContainer);
	}
}

void FAnimNode_NN::Evaluate_AnyThread(FPoseContext& Output)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Evaluate_AnyThread)
	Source.Evaluate(Output);
	const FBoneContainer& BoneContainer = Output.AnimInstanceProxy->GetRequiredBones();

	if (!isRunning) {
		Output.ResetToRefPose();
		return;
	}

	if (ModelData != nullptr && !isModelInitialized) {
		InitializeModel(ModelData);
	}

	float deltaTime = Output.AnimInstanceProxy->GetDeltaSeconds();

	TArray<float> FeatureVector = FeatureSet->ComputeFeaturesRealTime(BoneContainer, Output, deltaTime);

	if (FeatureVector.Num() == 0) {
		Output.ResetToRefPose();
		return;
	}

	int32 EvaluationResult = EvaluateModel(FeatureVector, deltaTime);

	if (EvaluationResult == 1) {
		if (static_cast<uint8>(FeatureSet->TransformType) & static_cast<uint8>(EFeatureBoneTransformFlags::Local)) {
			SetLocalBoneTransforms(Output, BoneContainer);
		}
		else if (static_cast<uint8>(FeatureSet->TransformType) & static_cast<uint8>(EFeatureBoneTransformFlags::ComponentSpace)){
			SetComponentSpaceBoneTransforms(Output, BoneContainer);
		}
	}
	else if (EvaluationResult == 0) {
		return;
	}
	else {
		Output.ResetToRefPose();
		return;
	}
}

void FAnimNode_NN::GatherDebugData(FNodeDebugData& DebugData)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(GatherDebugData)
	FString DebugLine = DebugData.GetNodeName(this);
	DebugData.AddDebugItem(DebugLine);
	Source.GatherDebugData(DebugData);
}

void FAnimNode_NN::InitializeModel(TObjectPtr<UNNEModelData> modelData) {
	TWeakInterfacePtr<INNERuntimeCPU> Runtime = UE::NNE::GetRuntime<INNERuntimeCPU>(FString("NNERuntimeORTCpu"));
	if (Runtime.IsValid()) {
		if (ModelData) {
			ModelInstance = MakeShared<FModelInstance>();
			ModelInstance->Initialize(modelData, Runtime);

			isModelInitialized = true;
		}
	}
}

int FAnimNode_NN::EvaluateModel(TArray<float>& InputData, const float DeltaTime) {
	if (ModelData != nullptr && !isModelInitialized) {
		InitializeModel(ModelData);
	}

	if (InputData.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("InputData is empty"));
		return -1;
	}

	if (isAsync) {
		if (!ModelInstance->bIsRunning) {
			if (ModelInstance->bIsFinished) {
				ModelInstance->bIsFinished = false;
				return ProcessOutput(DeltaTime);
			}

			ModelInstance->bIsRunning = true;
			TSharedPtr<FModelInstance> ModelInstancePtr = ModelInstance;
			TArray<float> InputDataCopy = InputData;
			AsyncTask(ENamedThreads::AnyNormalThreadNormalTask, [ModelInstancePtr, InputDataCopy]()
				{
					if (ModelInstancePtr->RunModel(InputDataCopy) != 0)
					{
						//UE_LOG(LogTemp, Error, TEXT("Failed to run the model"));
					}
					AsyncTask(ENamedThreads::GameThread, [ModelInstancePtr]()
						{
							ModelInstancePtr->bIsRunning = false;
							ModelInstancePtr->bIsFinished = true;
							//UE_LOG(LogTemp, Warning, TEXT("Model Run Successfully"))
						});
				});
			return 0;
		}
		return 0;

	}
	else {
		if (ModelInstance->RunModel(InputData) == 0) {
			UE_LOG(LogTemp, Warning, TEXT("ModelInstance->RunModel(InputData) == 0"));
			return -1;
		}
		return ProcessOutput(DeltaTime);
	}

	return -1;
}

void FAnimNode_NN::SetLocalBoneTransforms(FPoseContext& Output, const FBoneContainer& BoneContainer) {
	for (int i = 0; i < FeatureSet->OutputBones.Num(); i++) {
		const FCompactPoseBoneIndex CompactPoseBoneIndex = FeatureSet->OutputBones[i].GetCompactPoseIndex(BoneContainer);
		if (CompactPoseBoneIndex != INDEX_NONE) {
			FTransform& BoneTransform = Output.Pose[CompactPoseBoneIndex];
			BoneTransform.SetLocation(BonePositions[i]);
			BoneRotations[i].Normalize();
			BoneTransform.SetRotation(BoneRotations[i]);
			if (isInertialised) {
				Inertializers[i].Update(BoneTransform, Output.AnimInstanceProxy->GetDeltaSeconds());
			}
		}
	}
}

void FAnimNode_NN::SetComponentSpaceBoneTransforms(FPoseContext& Output, const FBoneContainer& BoneContainer) {

	FCSPose<FCompactPose> ComponentSpacePose;
	ComponentSpacePose.InitPose(Output.Pose);

	for (int i = 0; i < FeatureSet->OutputBones.Num(); i++) {
		const FCompactPoseBoneIndex CompactPoseBoneIndex = FeatureSet->OutputBones[i].GetCompactPoseIndex(BoneContainer);
		if (CompactPoseBoneIndex != INDEX_NONE) {
			FTransform BoneTransform = ComponentSpacePose.GetComponentSpaceTransform(CompactPoseBoneIndex);
			BoneTransform.SetLocation(BonePositions[i]);
			BoneRotations[i].Normalize();
			BoneTransform.SetRotation(BoneRotations[i]);
			if (isInertialised) {
				Inertializers[i].Update(BoneTransform, Output.AnimInstanceProxy->GetDeltaSeconds());
			}
			ComponentSpacePose.SetComponentSpaceTransform(CompactPoseBoneIndex, BoneTransform);
		}
	}

	FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(ComponentSpacePose, Output.Pose);
}

int FAnimNode_NN::ProcessOutput(const float DeltaTime) {
	TArray<float> output = ModelInstance->OutputData;
	if (output.Num() == 0) {
		UE_LOG(LogTemp, Warning, TEXT("OutputData is empty"));
		return -1;
	}

	if (output.Num() != FeatureSet->GetOutputVectorSize()) {
		UE_LOG(LogTemp, Warning, TEXT("Output format does not match database size"));
		return -1;
	}

	if (FeatureSet->OutputBones.Num() == 0) {
		UE_LOG(LogTemp, Warning, TEXT("OutputBones is empty"));
		return -1;
	}

	int outputIndex = 0;
	for (int i = 0; i < FeatureSet->OutputBones.Num(); i++) {
		if (static_cast<uint8>(FeatureSet->PropertiesToExtract) & static_cast<uint8>(EFeatureBoneFlags::Position))
		{
			FVector NewPosition = FVector(output[outputIndex], output[outputIndex + 1], output[outputIndex + 2]);
			outputIndex += 3;

			if (static_cast<uint8>(FeatureSet->PropertiesToExtract) & static_cast<uint8>(EFeatureBoneFlags::Velocity))
			{
				if (FeatureSet->bGetVelocitiesFromModelOutput) {
					BoneVelocities[i] = FVector(output[outputIndex], output[outputIndex + 1], output[outputIndex + 2]);
					outputIndex += 3;
				}
				else {
					BoneVelocities[i] = (NewPosition - BonePositions[i]) / DeltaTime;
				}
			}

			BonePositions[i] = NewPosition;
		}

		if (static_cast<uint8>(FeatureSet->PropertiesToExtract) & static_cast<uint8>(EFeatureBoneFlags::Rotation))
		{
			FQuat NewRotation = FQuat::Identity;
			switch (FeatureSet->RotationFormat)
			{
				case ERotationFormat::Quaternion:
				{
					NewRotation = FQuat(output[outputIndex], output[outputIndex + 1], output[outputIndex + 2], output[outputIndex + 3]);
					outputIndex += 4;
				}
				case ERotationFormat::XFormXY:
				{
					NewRotation = UFeatureComputation::GetQuatFromXformXY(FVector(output[outputIndex], output[outputIndex + 1], output[outputIndex + 2]), FVector(output[outputIndex + 3], output[outputIndex + 4], output[outputIndex + 5]));
					outputIndex += 6;
				}
			}

			if (static_cast<uint8>(FeatureSet->PropertiesToExtract) & static_cast<uint8>(EFeatureBoneFlags::AngularVelocity))
			{
				if (FeatureSet->bGetVelocitiesFromModelOutput)
				{
					BoneAngularVelocities[i] = FVector(output[outputIndex], output[outputIndex + 1], output[outputIndex + 2]);
					outputIndex += 3;
				}
				else
				{
					BoneAngularVelocities[i] = UFeatureComputation::QuatToScaledAngleAxis(NewRotation * BoneRotations[i].Inverse()) / DeltaTime;
				}
			}

			BoneRotations[i] = NewRotation;
		}
	}
	return 1;
}