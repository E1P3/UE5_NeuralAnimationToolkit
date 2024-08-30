// Fill out your copyright notice in the Description page of Project Settings.


#include "FeatureSetBuilder.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Animation/AnimSequenceDecompressionContext.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

void UFeatureSetBuilder::NativeConstruct()
{
	Super::NativeConstruct();

	if (LoadFeatureSetButton) {
		LoadFeatureSetButton->OnClicked.AddDynamic(this, &UFeatureSetBuilder::OnLoadFeatureSetButtonClicked);
	}

	if (NewBoneFeatureButton) {
		NewBoneFeatureButton->OnClicked.AddDynamic(this, &UFeatureSetBuilder::OnNewBoneFeatureButtonClicked);
	}

	if (NewTrajectoryFeatureButton) {
		NewTrajectoryFeatureButton->OnClicked.AddDynamic(this, &UFeatureSetBuilder::OnNewTrajectoryFeatureButtonClicked);
	}

	if (SaveFeatureSetButton) {
		SaveFeatureSetButton->OnClicked.AddDynamic(this, &UFeatureSetBuilder::OnSaveFeatureSetButtonClicked);
	}

	if (FeatureSetPropertyView) {
		FeatureSetPropertyView->SetPropertyName(GET_MEMBER_NAME_CHECKED(UFeatureSetBuilder, FeatureSetSchema));
		FeatureSetPropertyView->SetObject(this);
	}

	if (DetailsView) {
		DetailsView->SetObject(FeatureSetSchema);
		DetailsView->CategoriesToShow.Add(FName("Schema"));
		DetailsView->CategoriesToShow.Add(FName("Features"));
	}

	LoadFeatureList();  // Load existing features into the VerticalBox
}

void UFeatureSetBuilder::OnLoadFeatureSetButtonClicked()
{
	if (!FeatureSetSchema)
	{
		return;
	}

	if (DetailsView) {
		DetailsView->SetObject(FeatureSetSchema);
	}

	LoadFeatureList();  // Refresh the list of features
}

void UFeatureSetBuilder::OnNewBoneFeatureButtonClicked()
{
	if (!FeatureSetSchema)
	{
		return;
	}

	UBoneFeature* NewBoneFeature = NewObject<UBoneFeature>(FeatureSetSchema);
	FeatureSetSchema->AddFeature(NewBoneFeature);
	FeatureSetSchema->MarkPackageDirty();
	FeatureSetSchema->PostEditChange();

	LoadFeatureList();

}

void UFeatureSetBuilder::OnNewTrajectoryFeatureButtonClicked()
{
	if (!FeatureSetSchema)
	{
		return;
	}

	UTrajectoryFeature* NewTrajectoryFeature = NewObject<UTrajectoryFeature>(FeatureSetSchema);
	FeatureSetSchema->AddFeature(NewTrajectoryFeature);
	FeatureSetSchema->MarkPackageDirty();
	FeatureSetSchema->PostEditChange();

	LoadFeatureList();

}

void UFeatureSetBuilder::LoadFeatureList()
{
	if (!FeatureSetSchema || !FeatureListView)
	{
		return;
	}

	// Clear existing entries
	FeatureListView->ClearListItems();
	FeatureListView->SetListItems(FeatureSetSchema->GetFeatures());
}

void UFeatureSetBuilder::OnSaveFeatureSetButtonClicked()
{
	if (!FeatureSetSchema)
	{
		return;
	}

	// Get the package of the FeatureSet
	UPackage* Package = FeatureSetSchema->GetOutermost();
	if (!Package)
	{
		return;
	}

	// Define the path where the asset will be saved
	FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());

	// Save the package
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;
	SaveArgs.Error = GError;

	ESavePackageResult result = UPackage::Save(Package, FeatureSetSchema, *PackageFileName, SaveArgs).Result;
	bool bSuccess = result == ESavePackageResult::Success;

	if (bSuccess)
	{
		// Optionally, provide feedback to the user
		UE_LOG(LogTemp, Log, TEXT("Feature Set saved successfully: %s"), *PackageFileName);
	}
	else
	{
		// Optionally, provide feedback to the user
		UE_LOG(LogTemp, Error, TEXT("Failed to save Feature Set: %s"), *PackageFileName);
	}
}