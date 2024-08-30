// Fill out your copyright notice in the Description page of Project Settings.


#include "DatasetExtraction.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "BinaryBuilder.h"
#include "Animation/AnimSequenceDecompressionContext.h"


void UDatasetExtraction::NativeConstruct()
{
        Super::NativeConstruct();

        if (ExportButton)
        {
                ExportButton->OnClicked.AddDynamic(this, &UDatasetExtraction::OnExportButtonClicked);
                ExportFolderTextBox->OnTextChanged.AddDynamic(this, &UDatasetExtraction::OnExportFolderTextChanged);
                LoadFeatureSetButton->OnClicked.AddDynamic(this, &UDatasetExtraction::OnLoadFeatureSetButtonClicked);
                SelectAllBonesButton->OnClicked.AddDynamic(this, &UDatasetExtraction::OnSelectAllBonesButtonClicked);
                SelectAllAnimationsButton->OnClicked.AddDynamic(this, &UDatasetExtraction::OnSelectAllAnimationsButtonClicked);
		DeselectAllBonesButton->OnClicked.AddDynamic(this, &UDatasetExtraction::OnDeselectAllBonesButtonClicked);
		DeselectAllAnimationsButton->OnClicked.AddDynamic(this, &UDatasetExtraction::OnDeselectAllAnimationsButtonClicked);
        }

        if (FeatureSetPropertyView)
        {
                FeatureSetPropertyView->SetPropertyName(GET_MEMBER_NAME_CHECKED(UDatasetExtraction, FeatureSetSchema));
                FeatureSetPropertyView->SetObject(this);
        }

        if (DetailsView) {
                DetailsView->SetObject(FeatureSetSchema);
                DetailsView->CategoriesToShow.Add(FName("Features"));
                DetailsView->CategoriesToShow.Add(FName("Dataset"));
                DetailsView->CategoriesToShow.Add(FName("Schema"));
        }

}

// The export function used to compute the feature and dataset arrays and save them in the folder specified by the user
// The binary can the then read on python side through the BinaryReader.py script located in ExternalTools folder
// The format of the binary is specified in the BinaryBuilder.h file
void UDatasetExtraction::OnExportButtonClicked()
{
        if (FeatureSetSchema && AnimSequences.Num() > 0 && BoneInfo.Num() > 0)
        {
                UE_LOG(LogTemp, Warning, TEXT("Exporting data..."));

                TArray<UBoneInfoEntry*> SelectedBones;

                TArray<int32> dataset_dimensions;
		TArray<int32> featureset_dimensions;
                int32 boneCount = 0;
                int32 frameCount = 0;
                int32 featureSize = 0;
		int32 datasetVectorSize = 0;
		if (FeatureSetSchema) 
                { 
			datasetVectorSize = FeatureSetSchema->GetDatasetVectorSize();
                        featureSize = FeatureSetSchema->GetFeatureVectorSize(); 
			FeatureSetSchema->OutputBones.Empty();
                }

		TArray<float> Data;
                TArray<float> FeatureData;

                TArray<int32> ParentIndices = GetBoneParentIndices(BoneInfo);
                FString folderName = ExportFolderTextBox->GetText().ToString();
                FString filename = folderName == "" ? "parent_indices.bin" : folderName + "parent_indices.bin";

                TArray<int32> ParentIndicesDimensions;
                ParentIndicesDimensions.Add(ParentIndices.Num());

                UBinaryBuilder::SaveToBinaryFile(filename, ParentIndicesDimensions, ParentIndices);

		// Iterate through all bones and animation sequences selected in the widget to extract the bone transforms
                for (UBoneInfoEntry* Bone : BoneInfo) 
                {
                        if (Bone->bIsSelected) {
                                //UE_LOG(LogTemp, Warning, TEXT("Bone: %s"), *Bone->GetBoneName().ToString());
                                SelectedBones.Add(Bone);
                                if (FeatureSetSchema) {
                                        FBoneReference boneRef;
					boneRef.BoneName = Bone->GetBoneName();
					FeatureSetSchema->OutputBones.Add(boneRef);
                                }
                                boneCount++;
                        }
                }
                for (UAnimSequenceEntry* AnimSequence : AnimSequences) 
                {
                        if (AnimSequence->bIsSelected) {
                                UAnimSequence* AnimSequenceObj = AnimSequence->GetAnimSequence();

                                TArray<TArray<FTransform>> LocalBoneTransforms = GetBoneTransforms(AnimSequenceObj, BoneInfo);
                                TArray<TArray<FTransform>> ComponentSpaceBoneTransforms = RetrieveComponentSpaceTransforms(LocalBoneTransforms, BoneInfo);
                                if (FeatureSetSchema)
                                {
                                        if (static_cast<uint8>(FeatureSetSchema->TransformType) & static_cast<uint8>(EFeatureBoneTransformFlags::Local)) {
                                                Data.Append(SerializeBoneTransforms(LocalBoneTransforms, SelectedBones, AnimSequenceObj->GetPlayLength() / AnimSequenceObj->GetNumberOfSampledKeys()));
                                        }
                                        if (static_cast<uint8>(FeatureSetSchema->TransformType) & static_cast<uint8>(EFeatureBoneTransformFlags::ComponentSpace)) {
                                                Data.Append(SerializeBoneTransforms(ComponentSpaceBoneTransforms, SelectedBones, AnimSequenceObj->GetPlayLength() / AnimSequenceObj->GetNumberOfSampledKeys()));
                                        }
				        FeatureData.Append(FeatureSetSchema->ComputeFeaturesOffline(LocalBoneTransforms, ComponentSpaceBoneTransforms, AnimSequenceObj->GetPlayLength() / AnimSequenceObj->GetNumberOfSampledKeys()));
                                }
				frameCount += LocalBoneTransforms.Num();
                        }
                }

                // Save dataset
                dataset_dimensions.Add(frameCount);
                dataset_dimensions.Add(boneCount);
                dataset_dimensions.Add(datasetVectorSize);

                filename = folderName == "" ? "dataset.bin" : folderName + "dataset.bin";

		UBinaryBuilder::SaveToBinaryFile(filename, dataset_dimensions, Data);

		// Save featureset
		if (FeatureSetSchema) {
			featureset_dimensions.Add(frameCount);
			featureset_dimensions.Add(featureSize);

			filename = folderName == "" ? "features.bin" : folderName + "features.bin";

			UBinaryBuilder::SaveToBinaryFile(filename, featureset_dimensions, FeatureData);
		}

		UE_LOG(LogTemp, Warning, TEXT("Exported data to %s"), *filename);
		UE_LOG(LogTemp, Warning, TEXT("Exported %d frames, %d bones, %d features"), frameCount, boneCount, featureSize);


        }
}

// Loads the feature set and retrieves the bone information and animation sequences that reference the skeleton specified in the FeatureSetSchema
void UDatasetExtraction::OnLoadFeatureSetButtonClicked()
{
        FName PropertyName = FeatureSetPropertyView->GetPropertyName();

        if (PropertyName == GET_MEMBER_NAME_CHECKED(UDatasetExtraction, FeatureSetSchema))
        {
                if (FeatureSetSchema)
                {
                        Skeleton = FeatureSetSchema->Skeleton;

                        RetirieveBoneInfo();

                        RetirieveAnimSequences();

                        if (SequenceListView)
                        {
                                SequenceListView->SetListItems(AnimSequences);
                        }

                        if (BoneListView)
                        {
                                BoneListView->SetListItems(BoneInfo);
                        }
                }
        }

        if (DetailsView) {
                DetailsView->SetObject(FeatureSetSchema);
        }
}

void UDatasetExtraction::OnSelectAllBonesButtonClicked()
{
        for (UBoneInfoEntry* Bone : BoneInfo)
        {
                Bone->bIsSelected = true;
        }

        if (BoneListView)
        {
                BoneListView->ClearListItems();
                BoneListView->SetListItems(BoneInfo);
		TArray<UObject*> Items = BoneListView->GetListItems();

                for (UObject* Item : Items) {

			UListEntry* ListEntry = Cast<UListEntry>(BoneListView->GetEntryWidgetFromItem(Item));
                        if (ListEntry) {
                                ListEntry->SetCheckBox(true);
                        }
                }
        }
}

void UDatasetExtraction::OnSelectAllAnimationsButtonClicked()
{
        for (UAnimSequenceEntry* AnimSequence : AnimSequences)
        {
                AnimSequence->bIsSelected = true;
        }

        if (SequenceListView)
        {
                SequenceListView->ClearListItems();
                SequenceListView->SetListItems(AnimSequences);
                TArray<UObject*> Items = SequenceListView->GetListItems();

                for (UObject* Item : Items) {

                        UListEntry* ListEntry = Cast<UListEntry>(SequenceListView->GetEntryWidgetFromItem(Item));
                        if (ListEntry) {
                                ListEntry->SetCheckBox(true);
                        }
                }
        }
}

void UDatasetExtraction::OnDeselectAllBonesButtonClicked()
{
        for (UBoneInfoEntry* Bone : BoneInfo)
        {
                Bone->bIsSelected = false;
        }

        if (BoneListView)
        {
                BoneListView->ClearListItems();
                BoneListView->SetListItems(BoneInfo);
                TArray<UObject*> Items = BoneListView->GetListItems();

                for (UObject* Item : Items) {

                        UListEntry* ListEntry = Cast<UListEntry>(BoneListView->GetEntryWidgetFromItem(Item));
                        if (ListEntry) {
                                ListEntry->SetCheckBox(false);
                        }
                }
        }
}

void UDatasetExtraction::OnDeselectAllAnimationsButtonClicked()
{
        for (UAnimSequenceEntry* AnimSequence : AnimSequences)
        {
                AnimSequence->bIsSelected = false;
        }

        if (SequenceListView)
        {
                SequenceListView->ClearListItems();
                SequenceListView->SetListItems(AnimSequences);
                TArray<UObject*> Items = SequenceListView->GetListItems();

                for (UObject* Item : Items) {

                        UListEntry* ListEntry = Cast<UListEntry>(SequenceListView->GetEntryWidgetFromItem(Item));
                        if (ListEntry) {
                                ListEntry->SetCheckBox(false);
                        }
                }
        }
}


void UDatasetExtraction::OnExportFolderTextChanged(const FText& Text)
{
        ExportFolder = Text.ToString();
}

void UDatasetExtraction::RetirieveBoneInfo() {
        if (Skeleton) {

                BoneInfo.Empty();

                TArray<FMeshBoneInfo> BoneContainer = Skeleton->GetReferenceSkeleton().GetRefBoneInfo();

                for (int i = 0; i < BoneContainer.Num(); i++) {
                        UBoneInfoEntry* BoneInfoObj = NewObject<UBoneInfoEntry>();
                        BoneInfoObj->SetBone(BoneContainer[i].Name, i, BoneContainer[i].ParentIndex);
                        BoneInfo.Add(BoneInfoObj);
                }
        }
}

// Retrieve all animation sequences that reference the skeleton specified in the FeatureSetSchema
void UDatasetExtraction::RetirieveAnimSequences() {
        if (Skeleton) {

                AnimSequences.Empty();

                FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
                IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

                // Search for referencing packages to the currently open skeleton
                TArray<FAssetIdentifier> Referencers;
                AssetRegistry.GetReferencers(Skeleton->GetOuter()->GetFName(), Referencers);
                for (const FAssetIdentifier& Identifier : Referencers)
                {
                        TArray<FAssetData> Assets;
                        AssetRegistry.GetAssetsByPackageName(Identifier.PackageName, Assets);

                        for (const FAssetData& Asset : Assets)
                        {
                                // Only add assets whos class is of UAnimSequence
                                if (Asset.IsInstanceOf(UAnimSequence::StaticClass()))
                                {
                                        UAnimSequence* AnimSequence = CastChecked<UAnimSequence>(Asset.GetAsset());
                                        UAnimSequenceEntry* AnimSequenceObj = NewObject<UAnimSequenceEntry>();
                                        AnimSequenceObj->SetAnimSequence(AnimSequence);
                                        AnimSequences.Add(AnimSequenceObj);
                                }
                        }
                }
        }
}

// Retrieve frame-by-frame bone transforms for the selected bones in the selected animation sequence
TArray<TArray<FTransform>> UDatasetExtraction::GetBoneTransforms(UAnimSequence* AnimSequence, TArray<UBoneInfoEntry*> RequiredBones)
{
        TArray<TArray<FTransform>> AnimationData;

        FString out = " ";

        if (AnimSequence)
        {
                double CurrentTime = 0.0;
                double SequenceDuration = AnimSequence->GetPlayLength();
                int SequenceNumFrames = AnimSequence->GetNumberOfSampledKeys();
                double SequenceFrameRate = AnimSequence->GetPlayLength() / SequenceNumFrames;

                for (int i = 0; i < SequenceNumFrames; i++) {

                        out += FString::Printf(TEXT("Frame: %d\n"), i);

                        CurrentTime = i * SequenceFrameRate;

                        TArray<FTransform> BoneTransforms;

                        for (UBoneInfoEntry* Bone : RequiredBones) {
                                FReferenceSkeleton RefSkeleton = AnimSequence->GetSkeleton()->GetReferenceSkeleton();
                                if (FeatureSetSchema) {
					FeatureSetSchema->InitialiseFeaturesOffline(RefSkeleton);
                                }
                                int32 CurrentBoneIndex = RefSkeleton.FindBoneIndex(Bone->GetBoneName());
                                FSkeletonPoseBoneIndex SkeletonPoseBoneIndex = FSkeletonPoseBoneIndex(CurrentBoneIndex);
                                FTransform BoneTransform;
                                AnimSequence->GetBoneTransform(BoneTransform, SkeletonPoseBoneIndex, CurrentTime, false);

                                //out += FString::Printf(TEXT("Bone: %s, Position: %s\n"), *Bone->GetBoneName().ToString(), *BoneTransform.GetLocation().ToString());

                                BoneTransforms.Add(BoneTransform);
                        }

			AnimationData.Add(BoneTransforms);
                }
        }

        //UE_LOG(LogTemp, Warning, TEXT("%s"), *out);

        return AnimationData;
}

TArray<TArray<FTransform>> UDatasetExtraction::RetrieveComponentSpaceTransforms(TArray<TArray<FTransform>> BoneTransforms, TArray<UBoneInfoEntry*> RequiredBones) {
        TArray<TArray<FTransform>> ComponentSpaceTransforms;
        for (int i = 0; i < BoneTransforms.Num(); i++) {
	TArray<FTransform> ComponentSpaceFrame;
                for (int j = 0; j < BoneTransforms[0].Num(); j++) {
			FTransform ComponentSpaceTransform = BoneTransforms[i][j];
                        if (RequiredBones[j]->GetParentIndex() != -1) {
				FTransform ParentTransform = ComponentSpaceFrame[RequiredBones[j]->GetParentIndex()];
				ComponentSpaceTransform = ParentTransform.Inverse() * ComponentSpaceTransform;
			}
			ComponentSpaceFrame.Add(ComponentSpaceTransform);
		}
		ComponentSpaceTransforms.Add(ComponentSpaceFrame);
	}
	return ComponentSpaceTransforms;
}

// Writes all bone information into a one-dimensional float array to be saved in a binary file
TArray<float> UDatasetExtraction::SerializeBoneTransforms(const TArray<TArray<FTransform>>& BoneTransforms, const TArray<UBoneInfoEntry*> SelectedBones, const float frameRate) {
        TArray<float> Data;
        for (int i = 0; i < BoneTransforms.Num(); i++) {
                for (UBoneInfoEntry* Bone : SelectedBones) {

                        int32 BoneIndex = Bone->GetBoneIndex();

                        if (FeatureSetSchema) {
                                if (static_cast<uint8>(FeatureSetSchema->PropertiesToExtract) & static_cast<uint8>(EFeatureBoneFlags::Position))
                                {
					FVector position = BoneTransforms[i][BoneIndex].GetLocation();
					Data.Add(position.X);
					Data.Add(position.Y);
					Data.Add(position.Z);
				 }

                                if (static_cast<uint8>(FeatureSetSchema->PropertiesToExtract) & static_cast<uint8>(EFeatureBoneFlags::Rotation))
				{
					FQuat rotation = BoneTransforms[i][BoneIndex].GetRotation();
                                        switch (FeatureSetSchema->RotationFormat)
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

                                if (static_cast<uint8>(FeatureSetSchema->PropertiesToExtract) & static_cast<uint8>(EFeatureBoneFlags::Velocity))
                                {
                                        FVector velocity = FVector::ZeroVector;
                                        if (i == 0) {
                                                velocity = UFeatureComputation::GetBoneVelocity(BoneTransforms[i][BoneIndex], BoneTransforms[i][BoneIndex], BoneTransforms[i + 1][BoneIndex], frameRate);
                                        } else if (i == BoneTransforms.Num() - 1) {
						velocity = UFeatureComputation::GetBoneVelocity(BoneTransforms[i - 1][BoneIndex], BoneTransforms[i][BoneIndex], BoneTransforms[i][BoneIndex], frameRate);
					}
					else {
						velocity = UFeatureComputation::GetBoneVelocity(BoneTransforms[i - 1][BoneIndex], BoneTransforms[i][BoneIndex], BoneTransforms[i + 1][BoneIndex], frameRate);
					}
                                        Data.Add(velocity.X);
                                        Data.Add(velocity.Y);
                                        Data.Add(velocity.Z);
                                }

                                if (static_cast<uint8>(FeatureSetSchema->PropertiesToExtract) & static_cast<uint8>(EFeatureBoneFlags::AngularVelocity))
                                {
                                        FVector angularVelocity = FVector::ZeroVector;
                                        if (i == 0) {
						angularVelocity = UFeatureComputation::GetBoneAngularVelocity(BoneTransforms[i][BoneIndex], BoneTransforms[i][BoneIndex], BoneTransforms[i + 1][BoneIndex], frameRate);
					}
					else if (i == BoneTransforms.Num() - 1) {
						angularVelocity = UFeatureComputation::GetBoneAngularVelocity(BoneTransforms[i - 1][BoneIndex], BoneTransforms[i][BoneIndex], BoneTransforms[i][BoneIndex], frameRate);
					}
					else {
						angularVelocity = UFeatureComputation::GetBoneAngularVelocity(BoneTransforms[i - 1][BoneIndex], BoneTransforms[i][BoneIndex], BoneTransforms[i + 1][BoneIndex], frameRate);
					}
					Data.Add(angularVelocity.X);
					Data.Add(angularVelocity.Y);
					Data.Add(angularVelocity.Z);
                                }
                        }
                }

        }
        return Data;
}

TArray<int32> UDatasetExtraction::GetBoneParentIndices(const TArray<UBoneInfoEntry*> RequiredBones) {
        TArray<int32> ParentIndices;
        for (UBoneInfoEntry* Bone : RequiredBones) {
		ParentIndices.Add(Bone->GetParentIndex());
	}
	return ParentIndices;
}