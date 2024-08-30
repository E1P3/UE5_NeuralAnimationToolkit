#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "Features.h"
#include "Components/SinglePropertyView.h"
#include "Components/DetailsView.h"
#include "Components/TextBlock.h"
#include "EditorUtilityWidgetComponents.h"
#include "FeatureSetBuilder.generated.h"

// Widget to display the properties of each feature into a list
// Provides a way to easily display and modify the features in the feature set
UCLASS(Abstract)
class NEURALANIMATIONTOOLKIT_API UFeatureDetailsEntry : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, meta = (BindWidget))
	TObjectPtr<UDetailsView> DetailsView;

	// IUserObjectListEntry
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override {
		DetailsView->SetObject(ListItemObject);
	}
	// IUserObjectListEntry

private:
	TObjectPtr<UObject> Object;

};

// This widget provides all necessary functionality to load the featureSet dataAsset and design the feature set used in the animation system
// NOTE: Each time you introduce a new feature class, provide a new button to create a new feature of that class. Simply copy paste the code from the existing buttons and change the class name

UCLASS(Abstract)
class NEURALANIMATIONTOOLKIT_API UFeatureSetBuilder : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UFeatureSet* FeatureSetSchema = nullptr;

	UPROPERTY(EditAnywhere, meta = (BindWidget))
	TObjectPtr<UEditorUtilityButton> LoadFeatureSetButton;

	UPROPERTY(EditAnywhere, meta = (BindWidget))
	TObjectPtr<UEditorUtilityButton> NewBoneFeatureButton;

	UPROPERTY(EditAnywhere, meta = (BindWidget))
	TObjectPtr<UEditorUtilityButton> NewTrajectoryFeatureButton;

	UPROPERTY(EditAnywhere, meta = (BindWidget))
	TObjectPtr<UEditorUtilityButton> SaveFeatureSetButton;

	UPROPERTY(EditAnywhere, meta = (BindWidget))
	TObjectPtr<USinglePropertyView> FeatureSetPropertyView;

	UPROPERTY(EditAnywhere, meta = (BindWidget))
	TObjectPtr<UDetailsView> DetailsView;

	UPROPERTY(EditAnywhere, meta = (BindWidget))
	TObjectPtr<UEditorUtilityListView> FeatureListView;

	virtual void NativeConstruct() override;

private:

	UFUNCTION()
	void OnNewBoneFeatureButtonClicked(); // Button to create a new bone feature

	UFUNCTION()
	void OnNewTrajectoryFeatureButtonClicked(); // Button to create a new trajectory feature

	UFUNCTION()
	void OnLoadFeatureSetButtonClicked(); // Button to load the feature set and display its properties

	UFUNCTION()
	void OnSaveFeatureSetButtonClicked(); // Easy way to save the currently loaded feature instead if searching it in a folder

	void LoadFeatureList(); // Called each time the FeatureSet is modified to update the list of features
};
