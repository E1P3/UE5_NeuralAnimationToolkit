#pragma once

#include "BoneContainer.h"
#include "Engine/DataAsset.h"
#include "Interfaces/Interface_BoneReferenceSkeletonProvider.h"
#include "UObject/Interface.h"
#include "FeatureComputation.h"
#include "Features.generated.h"

struct FBoneReference;

// Bitmask enums for feature extraction to select in the editor
UENUM(meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true", ScriptName = "/Script/NNAnimationToolkit.EFeatureBoneFlags"))
enum class  EFeatureBoneFlags : uint8
{
	Position = 1 << 0,
	Rotation = 1 << 1,
	Velocity = 1 << 2,
	AngularVelocity = 1 << 3,
};
ENUM_CLASS_FLAGS(EFeatureBoneFlags);

UENUM(meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true", ScriptName = "/Script/NNAnimationToolkit.EFeatureBoneTransformFlags"))
enum class  EFeatureBoneTransformFlags : uint8
{
	ComponentSpace = 1 << 0,
	Local = 1 << 1,
};
ENUM_CLASS_FLAGS(EFeatureBoneTransformFlags);

UENUM(meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true", ScriptName = "/Script/NNAnimationToolkit.EFeatureTrajectoryFlags"))
enum class  EFeatureTrajectoryFlags : uint8
{
	Position = 1 << 0,
	Direction = 1 << 1,
};
ENUM_CLASS_FLAGS(EFeatureTrajectoryFlags);

UENUM(meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true", ScriptName = "/Script/NNAnimationToolkit.EFeatureTrajectoryDimensionFlags"))
enum class  EFeatureTrajectoryDimensionFlags : uint8
{
	Two = 1 << 0,
	Three = 1 << 1,
};
ENUM_CLASS_FLAGS(EFeatureTrajectoryDimensionFlags);

UENUM(BlueprintType) // Determines in which way the rotation is stored in the feature vector
enum class  ERotationFormat : uint8
{
	Quaternion        UMETA(DisplayName = "Quaternion"), // Quaternion representation of the rotation, not really used in practice when creating the dataset
	XFormXY     UMETA(DisplayName = "XFormXY"), // X and Y components of the rotation matrix
};

UCLASS()
class NEURALANIMATIONTOOLKIT_API UFeature : public UObject
{
	GENERATED_BODY()

public:
	// Interface functions to be implemented by all features
	// Each feature should support its own initialisation and computation both offline when extracting the dataset, and realtime when running the neural network
	virtual	void InitialiseOffline(const FReferenceSkeleton& RefSkeleton) {};
	virtual	void InitialiseRealTime(const FBoneContainer& BoneContainer)  {};
	virtual	TArray<float> ComputeRealTime(const FBoneContainer& BoneContainer, FCSPose<FCompactPose> InPose, float DeltaTime) { return TArray<float>(); };
	virtual	TArray<float> ComputeOffline(const TArray<TArray<FTransform>>& BoneTransforms, float DeltaTime, int FrameIndex) { return TArray<float>(); };
	virtual	int32 GetFeatureSize() const { return 0; } // Get the array size of the feature

	// Feature space to select between component space and local space
	UPROPERTY(EditAnywhere, meta = (Bitmask, BitmaskEnum = EFeatureBoneTransformFlags), Category = "Feature")
	int32 FeatureSpace = int32(EFeatureBoneTransformFlags::Local);
};
// Below are sample features showing how to use the interface to extract bone and trajectory related information
// When added to the feature set the interface funcitons are then used to compute the feature vetors necessary for the neural network

// Feature to extract bone related information
UCLASS()
class NEURALANIMATIONTOOLKIT_API UBoneFeature : public UFeature
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, Category = "Feature")
	FBoneReference BoneReference;

	UPROPERTY(EditAnywhere, meta = (Bitmask, BitmaskEnum = EFeatureBoneFlags), Category = "Feature")
	int32 Properties = int32(EFeatureBoneFlags::Position);

	UPROPERTY(EditAnywhere, Category = "Feature")
	ERotationFormat RotationFormat = ERotationFormat::XFormXY;

	UBoneFeature() = default;

	// IFeatureComputeInterface
	void InitialiseOffline(const FReferenceSkeleton& RefSkeleton) override { BoneIndex = UFeatureComputation::GetBoneIndex(RefSkeleton, BoneReference); }
	void InitialiseRealTime(const FBoneContainer& BoneContainer) override { 
		BoneReference.Initialize(BoneContainer);
		BoneIndex = int32(BoneReference.GetCompactPoseIndex(BoneContainer));
	};
	TArray<float> ComputeRealTime(const FBoneContainer& BoneContainer, FCSPose<FCompactPose> InPose, float DeltaTime) override
	{
		TArray<float> Data;

		if (BoneIndex == INDEX_NONE) return Data;

		bool isLocal = static_cast<uint8>(FeatureSpace) & static_cast<uint8>(EFeatureBoneTransformFlags::Local);

		FTransform BoneTransform = isLocal ? InPose.GetLocalSpaceTransform(FCompactPoseBoneIndex(BoneIndex)) : InPose.GetComponentSpaceTransform(FCompactPoseBoneIndex(BoneIndex));

		FVector position = BoneTransform.GetLocation();
		FQuat rotation = BoneTransform.GetRotation();

		if (static_cast<uint8>(Properties) & static_cast<uint8>(EFeatureBoneFlags::Position))
		{
			
			Data.Add(position.X);
			Data.Add(position.Y);
			Data.Add(position.Z);
		}

		if (static_cast<uint8>(Properties) & static_cast<uint8>(EFeatureBoneFlags::Rotation))
		{
			
			switch (RotationFormat)
			{
				case ERotationFormat::Quaternion:
				{
					Data.Add(rotation.X);
					Data.Add(rotation.Y);
					Data.Add(rotation.Z);
					Data.Add(rotation.W);
					break;
				}
				case ERotationFormat::XFormXY:
				{
					FVector x, y;
					UFeatureComputation::GetXformXYFromQuat(rotation, x, y);
					Data.Add(x.X);
					Data.Add(x.Y);
					Data.Add(x.Z);
					Data.Add(y.X);
					Data.Add(y.Y);
					Data.Add(y.Z);
					break;
				}
			}
		}

		if (static_cast<uint8>(Properties) & static_cast<uint8>(EFeatureBoneFlags::Velocity)) {
			FVector prevPosition = CachedBoneTransform.GetLocation();
			FVector velocity = position - prevPosition / DeltaTime;
			Data.Add(velocity.X);
			Data.Add(velocity.Y);
			Data.Add(velocity.Z);
		}

		if (static_cast<uint8>(Properties) & static_cast<uint8>(EFeatureBoneFlags::AngularVelocity)) {
			FVector prevRotation = UFeatureComputation::QuatToScaledAngleAxis(CachedBoneTransform.GetRotation());
			FVector currentRotation = UFeatureComputation::QuatToScaledAngleAxis(rotation);
			FVector angularVelocity = (currentRotation - prevRotation) / DeltaTime;
			Data.Add(angularVelocity.X);
			Data.Add(angularVelocity.Y);
			Data.Add(angularVelocity.Z);
		}

		CachedBoneTransform = BoneTransform;

		return Data;
	}

	TArray<float> ComputeOffline(const TArray<TArray<FTransform>>& BoneTransforms, float DeltaTime, int FrameIndex) override
	{
		if (BoneIndex == INDEX_NONE) return TArray<float>();

		TArray<float> Data;

		if (static_cast<uint8>(Properties) & static_cast<uint8>(EFeatureBoneFlags::Position))
		{
			FVector position = BoneTransforms[FrameIndex][BoneIndex].GetLocation();
			Data.Add(position.X);
			Data.Add(position.Y);
			Data.Add(position.Z);
		}

		if (static_cast<uint8>(Properties) & static_cast<uint8>(EFeatureBoneFlags::Rotation))
		{
			FQuat rotation = BoneTransforms[FrameIndex][BoneIndex].GetRotation();
			switch (RotationFormat)
			{
				case ERotationFormat::Quaternion:
				{
					Data.Add(rotation.X);
					Data.Add(rotation.Y);
					Data.Add(rotation.Z);
					Data.Add(rotation.W);
					break;
				}
				case ERotationFormat::XFormXY:
				{
					FVector x, y;
					UFeatureComputation::GetXformXYFromQuat(rotation, x, y);
					Data.Add(x.X);
					Data.Add(x.Y);
					Data.Add(x.Z);
					Data.Add(y.X);
					Data.Add(y.Y);
					Data.Add(y.Z);
					break;
				}
			}
		}

		if (static_cast<uint8>(Properties) & static_cast<uint8>(EFeatureBoneFlags::Velocity))
		{
			FVector velocity = FVector::ZeroVector;
			if (FrameIndex == 0) {
				velocity = UFeatureComputation::GetBoneVelocity(BoneTransforms[FrameIndex][BoneIndex], BoneTransforms[FrameIndex][BoneIndex], BoneTransforms[FrameIndex+ 1][BoneIndex], DeltaTime);
			}
			else if (FrameIndex == BoneTransforms.Num() - 1) {
				velocity = UFeatureComputation::GetBoneVelocity(BoneTransforms[FrameIndex- 1][BoneIndex], BoneTransforms[FrameIndex][BoneIndex], BoneTransforms[FrameIndex][BoneIndex], DeltaTime);
			}
			else {
				velocity = UFeatureComputation::GetBoneVelocity(BoneTransforms[FrameIndex- 1][BoneIndex], BoneTransforms[FrameIndex][BoneIndex], BoneTransforms[FrameIndex+ 1][BoneIndex], DeltaTime);
			}
			Data.Add(velocity.X);
			Data.Add(velocity.Y);
			Data.Add(velocity.Z);
		}

		if (static_cast<uint8>(Properties) & static_cast<uint8>(EFeatureBoneFlags::AngularVelocity))
		{
			FVector angularVelocity = FVector::ZeroVector;
			if (FrameIndex == 0) {
				angularVelocity = UFeatureComputation::GetBoneAngularVelocity(BoneTransforms[FrameIndex][BoneIndex], BoneTransforms[FrameIndex][BoneIndex], BoneTransforms[FrameIndex+ 1][BoneIndex], DeltaTime);
			}
			else if (FrameIndex == BoneTransforms.Num() - 1) {
				angularVelocity = UFeatureComputation::GetBoneAngularVelocity(BoneTransforms[FrameIndex- 1][BoneIndex], BoneTransforms[FrameIndex][BoneIndex], BoneTransforms[FrameIndex][BoneIndex], DeltaTime);
			}
			else {
				angularVelocity = UFeatureComputation::GetBoneAngularVelocity(BoneTransforms[FrameIndex- 1][BoneIndex], BoneTransforms[FrameIndex][BoneIndex], BoneTransforms[FrameIndex + 1][BoneIndex], DeltaTime);
			}
			Data.Add(angularVelocity.X);
			Data.Add(angularVelocity.Y);
			Data.Add(angularVelocity.Z);
		}

		return Data;
	}

	int32 GetFeatureSize() const override
	{
		int32 Size = 0;
		if (static_cast<uint8>(Properties) & static_cast<uint8>(EFeatureBoneFlags::Position)) 
		{ 
			Size += 3; 
		}
		if (static_cast<uint8>(Properties) & static_cast<uint8>(EFeatureBoneFlags::Rotation)) 
		{ 
			if (RotationFormat == ERotationFormat::Quaternion)
			{
				Size += 4;
			}
			else if (RotationFormat == ERotationFormat::XFormXY)
			{
				Size += 6;
			}
		}
		if (static_cast<uint8>(Properties) & static_cast<uint8>(EFeatureBoneFlags::Velocity)) 
		{ 
			Size += 3; 
		}
		if (static_cast<uint8>(Properties) & static_cast<uint8>(EFeatureBoneFlags::AngularVelocity)) 
		{ 
			Size += 3; 
		}
		return Size;
	}
	// End IFeatureComputeInterface

private:
	int32 BoneIndex = INDEX_NONE;
	FTransform CachedBoneTransform;
};

// Feature to extract trajectory related information
UCLASS()
class NEURALANIMATIONTOOLKIT_API UTrajectoryFeature : public UFeature
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, Category = "Feature")
	FBoneReference PositionBoneReference;

	UPROPERTY(EditAnywhere, Category = "Feature")
	FBoneReference DirectionBoneReference;

	UPROPERTY(EditAnywhere, meta = (Bitmask, BitmaskEnum = EFeatureTrajectoryFlags), Category = "Feature")
	int32 Property = int32(EFeatureTrajectoryFlags::Position);

	UPROPERTY(EditAnywhere, meta = (Bitmask, BitmaskEnum = EFeatureTrajectoryDimensionFlags), Category = "Feature")
	int32 Dimension = int32(EFeatureTrajectoryDimensionFlags::Two);

	UPROPERTY(EditAnywhere, Category = "Feature")
	int32 NumSamples = 3;

	UPROPERTY(EditAnywhere, Category = "Feature")
	float SamplingRate = 0.3f;

	UTrajectoryFeature()
	{
		PositionBoneReference.BoneName = "None";
		DirectionBoneReference.BoneName = "None";
	}

	void InitialiseOffline(const FReferenceSkeleton& RefSkeleton) override
	{
		PositionBoneIndex = UFeatureComputation::GetBoneIndex(RefSkeleton, PositionBoneReference);
		DirectionBoneIndex = UFeatureComputation::GetBoneIndex(RefSkeleton, DirectionBoneReference);
	}
	void InitialiseRealTime(const FBoneContainer& BoneContainer) override { 
		PositionBoneReference.Initialize(BoneContainer);
		DirectionBoneReference.Initialize(BoneContainer);
		PositionBoneIndex = int32(PositionBoneReference.GetCompactPoseIndex(BoneContainer));
		DirectionBoneIndex = int32(DirectionBoneReference.GetCompactPoseIndex(BoneContainer));
	}

	TArray<float> ComputeRealTime(const FBoneContainer& BoneContainer, FCSPose<FCompactPose> InPose, float DeltaTime) override 
	{ 
		TArray<float> Data = TArray<float>();
		if (static_cast<uint8>(Property) & static_cast<uint8>(EFeatureTrajectoryFlags::Position))
		{
			FVector position;
			if (PositionBoneIndex != INDEX_NONE)
			{
				position = InPose.GetComponentSpaceTransform(FCompactPoseBoneIndex(PositionBoneIndex)).GetLocation();
			}
			Data.Add(position.X);
			Data.Add(position.Y);
			if (static_cast<uint8>(Dimension) & static_cast<uint8>(EFeatureTrajectoryDimensionFlags::Three))
			{
				Data.Add(position.Z);
			}
		}
		if (static_cast<uint8>(Property) & static_cast<uint8>(EFeatureTrajectoryFlags::Direction))
		{
			FQuat rotation;
			if (DirectionBoneIndex != INDEX_NONE)
			{
				rotation = InPose.GetComponentSpaceTransform(FCompactPoseBoneIndex(DirectionBoneIndex)).GetRotation();
			}

			FVector direction = rotation.RotateVector(FVector(1.0f, 0.0f, 0.0f));
			Data.Add(direction.X);
			Data.Add(direction.Y);
			if (static_cast<uint8>(Dimension) & static_cast<uint8>(EFeatureTrajectoryDimensionFlags::Three))
			{
				Data.Add(direction.Z);
			}

		}
		return Data;
	}

	TArray<float> ComputeOffline(const TArray<TArray<FTransform>>& BoneTransforms, float DeltaTime, int FrameIndex) override 
	{
		float SamplingIndexOffset = SamplingRate / DeltaTime;

		TArray<float> Data;
		
		for (int i = 0; i < NumSamples; i++)
		{
			float SampleFloatIndex = FrameIndex + i * SamplingIndexOffset;
			int32 SampleIndex = FMath::FloorToInt(SampleFloatIndex); 
			float Fraction = SampleFloatIndex - SampleIndex;         

			int32 Index1 = FMath::Clamp(SampleIndex, 0, BoneTransforms.Num() - 1);
			int32 Index2 = FMath::Clamp(SampleIndex + 1, 0, BoneTransforms.Num() - 1);

			if (static_cast<uint8>(Property) & static_cast<uint8>(EFeatureTrajectoryFlags::Position))
			{
				FVector P1 = BoneTransforms[Index1][PositionBoneIndex].GetLocation();
				FVector P2 = BoneTransforms[Index2][PositionBoneIndex].GetLocation();

				FVector InterpolatedPosition = FMath::Lerp(P1, P2, Fraction);

				Data.Add(InterpolatedPosition.X);
				Data.Add(InterpolatedPosition.Y);

				if (static_cast<uint8>(Dimension) & static_cast<uint8>(EFeatureTrajectoryDimensionFlags::Three))
				{
					Data.Add(InterpolatedPosition.Z);
				}
			}

			if (static_cast<uint8>(Property) & static_cast<uint8>(EFeatureTrajectoryFlags::Direction))
			{
				FQuat P1 = BoneTransforms[Index1][DirectionBoneIndex].GetRotation();
				FQuat P2 = BoneTransforms[Index2][DirectionBoneIndex].GetRotation();

				FQuat InterpolatedDirection = FQuat::Slerp(P1, P2, Fraction);

				FVector InterpolatedDirectionVector = InterpolatedDirection.RotateVector(FVector(1.0f, 0.0f, 0.0f));

				Data.Add(InterpolatedDirection.X);
				Data.Add(InterpolatedDirection.Y);

				if (static_cast<uint8>(Dimension) & static_cast<uint8>(EFeatureTrajectoryDimensionFlags::Three))
				{
					Data.Add(InterpolatedDirection.Z);
				}
			}
		}

		return Data;
	}

	int32 GetFeatureSize() const override
	{
		int32 Size = 0;
		if (static_cast<uint8>(Property) & static_cast<uint8>(EFeatureTrajectoryFlags::Position)) Size += 3 * NumSamples;
		if (static_cast<uint8>(Property) & static_cast<uint8>(EFeatureTrajectoryFlags::Direction)) Size += 3 * NumSamples;
		return Size;
	}

private:
	int32 PositionBoneIndex = INDEX_NONE;
	int32 DirectionBoneIndex = INDEX_NONE;
};


// Feature set data asset to store all features and interact with toolkit widgets and the neural network animNode
// Offline, this class is used to generate the dataset and feature files that can be then passed to the python side of the project using a parser located in ExternalTools folder.
UCLASS(BlueprintType, Category = "Animation|Feature Set", Experimental, meta = (DisplayName = "Feature Set Config"), CollapseCategories)
class NEURALANIMATIONTOOLKIT_API UFeatureSet : public UDataAsset, public IBoneReferenceSkeletonProvider
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, Category = "Schema", meta = (DisplayPriority = 0))
	TObjectPtr<USkeleton> Skeleton;

	UPROPERTY(EditAnywhere, meta = (Bitmask, BitmaskEnum = EFeatureBoneFlags), Category = "Dataset")
	int32 PropertiesToExtract = int32(EFeatureBoneFlags::Position);

	UPROPERTY(EditAnywhere, meta = (Bitmask, BitmaskEnum = EFeatureBoneTransformFlags), Category = "Dataset")
	int32 TransformType = int32(EFeatureBoneTransformFlags::Local);

	UPROPERTY(EditAnywhere, Category = "Dataset")
	ERotationFormat RotationFormat = ERotationFormat::XFormXY;

	UPROPERTY(EditAnywhere, Category = "Dataset")
	TArray<FBoneReference> OutputBones;

	UPROPERTY(EditAnywhere, Category = "Dataset")
	bool bGetVelocitiesFromModelOutput = false;

	// IBoneReferenceSkeletonProvider
	USkeleton* GetSkeleton(bool& bInvalidSkeletonIsError, const IPropertyHandle* PropertyHandle) override
	{
		bInvalidSkeletonIsError = false;
		return Skeleton;
	}

	UFUNCTION(BlueprintCallable)
	USkeleton* GetSkeletonReference() { return Skeleton; }

	UFUNCTION(BlueprintCallable)
	void AddFeature(UFeature* Feature)
	{
		Features.Add(Feature);
	}

	TArray<TObjectPtr<UFeature>> GetFeatures() const { return Features; }

	void InitialiseFeaturesOffline(const FReferenceSkeleton& RefSkeleton)
	{
		for (TObjectPtr<UFeature> Feature : Features)
		{
			Feature->InitialiseOffline(RefSkeleton);
		}
	}

	void InitialiseFeaturesRealTime(const FBoneContainer& BoneContainer)
	{
		for (TObjectPtr<UFeature> Feature : Features)
		{
			Feature->InitialiseRealTime(BoneContainer);
		}
	}

	TArray<float> ComputeFeaturesOffline(const TArray<TArray<FTransform>>& LocalBoneTransforms, const TArray<TArray<FTransform>>& ComponentSpaceBoneTransforms, float DeltaTime) {
		TArray<float> FeatureVector;
		for (int i = 0; i < LocalBoneTransforms.Num(); i++)
		{
			TArray<float> FrameData;
			for (TObjectPtr<UFeature> Feature : Features)
			{
				if (static_cast<uint8>(Feature->FeatureSpace) & static_cast<uint8>(EFeatureBoneTransformFlags::Local))
				{
					TArray<float> FeatureData = Feature->ComputeOffline(LocalBoneTransforms, DeltaTime, i);
					FrameData.Append(FeatureData);
				}

				if (static_cast<uint8>(Feature->FeatureSpace) & static_cast<uint8>(EFeatureBoneTransformFlags::ComponentSpace))
				{
					TArray<float> FeatureData = Feature->ComputeOffline(ComponentSpaceBoneTransforms, DeltaTime, i);
					FrameData.Append(FeatureData);
				}
			}
			FeatureVector.Append(FrameData);
		}
		return FeatureVector;
	}

	TArray<float> ComputeFeaturesRealTime(const FBoneContainer& BoneContainer, FPoseContext& Output, float DeltaTime) {
		FCSPose<FCompactPose> CurrentPose;
		CurrentPose.InitPose(Output.Pose);

		TArray<float> FeatureVector;
		for (TObjectPtr<UFeature> Feature : Features)
		{
			TArray<float> FeatureData = Feature->ComputeRealTime(BoneContainer, CurrentPose, DeltaTime);
			FeatureVector.Append(FeatureData);
		}
		return FeatureVector;
	}

	int32 GetFeatureVectorSize() const
	{
		int32 Size = 0;
		for (TObjectPtr<UFeature> Feature : Features)
		{
			Size += Feature->GetFeatureSize();

		}
		return Size;
	}

	int32 GetDatasetVectorSize() const
	{
		int32 Size = 0;
		if (static_cast<uint8>(PropertiesToExtract) & static_cast<uint8>(EFeatureBoneFlags::Position)) 
		{ 
			Size += 3; 
		}
		if (static_cast<uint8>(PropertiesToExtract) & static_cast<uint8>(EFeatureBoneFlags::Rotation))
		{ 
			if (RotationFormat == ERotationFormat::Quaternion)
			{
				Size += 4;
			}
			else if (RotationFormat == ERotationFormat::XFormXY)
			{
				Size += 6;
			}
		}
		if (static_cast<uint8>(PropertiesToExtract) & static_cast<uint8>(EFeatureBoneFlags::Velocity))
		{ 
			Size += 3; 
		}
		if (static_cast<uint8>(PropertiesToExtract) & static_cast<uint8>(EFeatureBoneFlags::AngularVelocity))
		{ 
			Size += 3; 
		}
		return Size * OutputBones.Num();
	}

	int32 GetOutputVectorSize() const
	{
		int32 Size = 0;
		if (static_cast<uint8>(PropertiesToExtract) & static_cast<uint8>(EFeatureBoneFlags::Position))
		{
			Size += 3;
		}
		if (static_cast<uint8>(PropertiesToExtract) & static_cast<uint8>(EFeatureBoneFlags::Rotation))
		{
			if (RotationFormat == ERotationFormat::Quaternion)
			{
				Size += 4;
			}
			else if (RotationFormat == ERotationFormat::XFormXY)
			{
				Size += 6;
			}
		}
		if (static_cast<uint8>(PropertiesToExtract) & static_cast<uint8>(EFeatureBoneFlags::Velocity) && bGetVelocitiesFromModelOutput)
		{
			Size += 3;
		}
		if (static_cast<uint8>(PropertiesToExtract) & static_cast<uint8>(EFeatureBoneFlags::AngularVelocity) && bGetVelocitiesFromModelOutput)
		{
			Size += 3;
		}
		return Size * OutputBones.Num();
	}

private:

	// Array of features defined in the feature set
	// Make sure all the features located here implement the interface
	UPROPERTY(EditAnywhere, Category = "Features")
	TArray<TObjectPtr<UFeature>> Features;
};
