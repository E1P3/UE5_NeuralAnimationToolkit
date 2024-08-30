// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "Features.h"
#include "Components/SinglePropertyView.h"
#include "Components/DetailsView.h"
#include "Components/TextBlock.h"
#include "EditorUtilityWidgetComponents.h"
#include "DatasetExtraction.generated.h"

// Entry classes passe to the UListEntry widget
// They are used to represent the data in the list view
// The ListEntry can then be used in the list view in DatasetExtraction widget
// The provide all the necessary information to display the entry in the list view and connect it to the UObject it represents
UCLASS(Abstract)
class UListEntry_Base : public UObject
{
        GENERATED_BODY()

public:

        UListEntry_Base() {}
        UListEntry_Base(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        FText EntryText = FText::FromString(TEXT("Entry Text"));

        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        bool bIsSelected = false;
};

UCLASS()
class UAnimSequenceEntry : public UListEntry_Base
{
        GENERATED_BODY()

public:
        UAnimSequenceEntry() {}
        UAnimSequenceEntry(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

        UAnimSequenceEntry(UAnimSequence* InAnimSequence)
        {
                AnimSequence = InAnimSequence;
                EntryText = FText::FromName(AnimSequence->GetFName());
        }

        void SetAnimSequence(UAnimSequence* InAnimSequence)
        {
                AnimSequence = InAnimSequence;
                EntryText = FText::FromName(AnimSequence->GetFName());
        }

        UAnimSequence* GetAnimSequence() const { return AnimSequence; }

protected:
        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        UAnimSequence* AnimSequence;

};

UCLASS()
class UBoneInfoEntry : public UListEntry_Base
{
        GENERATED_BODY()

public:
        UBoneInfoEntry() {}
        UBoneInfoEntry(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

        UBoneInfoEntry(FName InBoneName, int32 InBoneIndex, int32 InParentIndex)
        {
                BoneName = InBoneName;
                BoneIndex = InBoneIndex;
                ParentIndex = InParentIndex;
                EntryText = FText::FromName(BoneName);
        }

        void SetBone(FName InBoneName, int32 InBoneIndex, int32 InParentIndex)
        {
                BoneName = InBoneName;
                BoneIndex = InBoneIndex;
                ParentIndex = InParentIndex;
                EntryText = FText::FromName(BoneName);
        }

        FName GetBoneName() const { return BoneName; }
        int32 GetBoneIndex() const { return BoneIndex; }
        int32 GetParentIndex() const { return ParentIndex; }

protected:
        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        FName BoneName;

        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        int32 BoneIndex;

        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        int32 ParentIndex;

};


// Custom widget to display a simple list of animations or bone information in the DatasetExtraction widget
UCLASS(Abstract)
class UListEntry : public UUserWidget, public IUserObjectListEntry
{
        GENERATED_BODY()

        virtual void NativeConstruct() override
        {
                Super::NativeConstruct();
                CheckBox->OnCheckStateChanged.AddDynamic(this, &UListEntry::OnCheckBoxStateChanged);
        }

public:
        void SetCheckBox(bool bIsChecked)
        {
                if (CheckBox)
                {
                        CheckBox->SetIsChecked(bIsChecked);
                }
        }

protected:
        // IUserObjectListEntry
        virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override {

                UListEntry_Base* EntryItem = Cast<UListEntry_Base>(ListItemObject);

                if (EntryItem)
                {
                        NameLabel->SetText(EntryItem->EntryText);
                        Entry = EntryItem;

                        if (CheckBox)
                        {
                                CheckBox->SetIsChecked(EntryItem->bIsSelected);
                        }
                }

        }
        // IUserObjectListEntry

        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        UListEntry_Base* Entry;

        UPROPERTY(meta = (BindWidget))
        class UTextBlock* NameLabel;

        UPROPERTY(meta = (BindWidget))
        class UEditorUtilityCheckBox* CheckBox;

        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        FText Title;

private:

        UFUNCTION()
        void OnCheckBoxStateChanged(bool bIsChecked)
        {
                Entry->bIsSelected = bIsChecked;
        }

};

// The main widget for extracting dataset from animations
// Based on the provided featureset it displays all bones and animations of the skeleton and provides a simple way to select the bones and animations to extract into a dataset and the featureset
UCLASS(Abstract)
class UDatasetExtraction : public UEditorUtilityWidget
{
        GENERATED_BODY()

public:

        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        USkeleton* Skeleton;

        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        UFeatureSet* FeatureSetSchema = nullptr;

        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        FString ExportFolder;

        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        TArray<UAnimSequenceEntry*> AnimSequences;

        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        TArray<UBoneInfoEntry*> BoneInfo;

        UPROPERTY(EditAnywhere, meta = (BindWidget))
        TObjectPtr<UEditorUtilityButton> ExportButton;

        UPROPERTY(EditAnywhere, meta = (BindWidget))
        TObjectPtr<UEditorUtilityButton> LoadFeatureSetButton;

        UPROPERTY(EditAnywhere, meta = (BindWidget))
        TObjectPtr<UEditorUtilityButton> SelectAllBonesButton;

        UPROPERTY(EditAnywhere, meta = (BindWidget))
        TObjectPtr<UEditorUtilityButton> SelectAllAnimationsButton;

        UPROPERTY(EditAnywhere, meta = (BindWidget))
        TObjectPtr<UEditorUtilityButton> DeselectAllBonesButton;

        UPROPERTY(EditAnywhere, meta = (BindWidget))
        TObjectPtr<UEditorUtilityButton> DeselectAllAnimationsButton;

        UPROPERTY(EditAnywhere, meta = (BindWidget))
        TObjectPtr<UEditorUtilityEditableTextBox> ExportFolderTextBox;

        UPROPERTY(EditAnywhere, meta = (BindWidget))
	TObjectPtr<USinglePropertyView> FeatureSetPropertyView; // Used to display the FeatureSet properties. To change the schema use the FeatureBuilder Widget instead

        UPROPERTY(EditAnywhere, meta = (BindWidget))
        TObjectPtr<UDetailsView> DetailsView;

        UPROPERTY(EditAnywhere, meta = (BindWidget))
        TObjectPtr<UEditorUtilityListView> SequenceListView;

        UPROPERTY(EditAnywhere, meta = (BindWidget))
        TObjectPtr<UEditorUtilityListView> BoneListView;

        virtual void NativeConstruct() override;

private:

        UFUNCTION()
        void OnExportButtonClicked();

        UFUNCTION()
        void OnLoadFeatureSetButtonClicked();

        UFUNCTION()
        void OnSelectAllBonesButtonClicked();

        UFUNCTION()
        void OnSelectAllAnimationsButtonClicked();

        UFUNCTION()
        void OnDeselectAllBonesButtonClicked();

        UFUNCTION()
        void OnDeselectAllAnimationsButtonClicked();

        UFUNCTION()
        void OnExportFolderTextChanged(const FText& Text);

        void RetirieveBoneInfo();

        void RetirieveAnimSequences();

        TArray<TArray<FTransform>> GetBoneTransforms(UAnimSequence* AnimSequence, TArray<UBoneInfoEntry*> RequiredBones);
        TArray<TArray<FTransform>> RetrieveComponentSpaceTransforms(TArray<TArray<FTransform>> BoneTransforms, TArray<UBoneInfoEntry*> RequiredBones);
	TArray<float> SerializeBoneTransforms(const TArray<TArray<FTransform>>& BoneTransforms, const TArray<UBoneInfoEntry*> SelectedBones, const float frameRate);
        TArray<int32> GetBoneParentIndices(const TArray<UBoneInfoEntry*> RequiredBones);
};
