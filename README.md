# Neural Animation Toolkit

This project provides a set of tools aiming to make it easier to experiment with running animations from neural network models. By unifying the offline and realtime feature definitions, the toolkit should make it easier to extract the dataset from the engine directly and run the models trained on that dataset in real time through custom animation node.

The project is intended to be the baseline to be extended for your chosen use case. It provides all the template code you need for training and running the model of your choice

The plugin contains the following features

- Feature Data Asset for defining features based on the skeleton and chosen parameters
- Feature Builder Widget for defining the feature vector of your model
- Dataset Extraction Widget for convenient extraction of the animation data straight from the engine
- Sample Animation Node for running the model
- Inertialiser for smoothing inbetween frames

## How it works
### Create Feature Set Data Asset
Defining the model requires establishing features to extract and append to every frame in the dataset. Start with creating Data Asset and select *Feature Set Config*. This will be ther feature set object used for extracting the dataset and running the network in the animation blueprint.

![Data Asset](/Media/dataAsset.png)

### Modify Features through Feature Builder Widget

The widgets used in the plugin are located in the content folder

![Widgets](/Media/WidgetLocation.png)

To modify the feature set open the FeatureBuilder widget.

![FeatureBuilder](/Media/Feature%20Builder.png)

1. Select the Feature Set Data Asset from the asset registry
2. Load the dataset into the widget to display its properties
3. Select the skeleton you are going to define your model on
4. Add Features from the given list
5. Modify feature properties
6. Save the Feature Set

### Extracting the dataset through Dataset Extractor Widget

To extract the features run the DatasetExtraction Widget from the same location

![DatasetExtraction](/Media/DataExtraction.png)

1. Select the Feature Set from asset registry 
2. Load the feature set into the widget. The bones of the chosen skeleton as well as all the animations of said skeleton found in the project will appear
3. Modify the dataset properties based on what form of the animation data you want to extract
4. Select the bones to extract
5. Select the animations to extract the frames from
6. Specify the save location
7. Export the binaries into the chosen folder

### Parse the dataset into python
The dataset extracted will consist of three binary files

* The dataset containing all frame data of each specified bone
* The computed feature set matching each frame in the dataset
* The parent indicies for computing loss values etc

These files are saved in a format as shown here

**[dimension array size][dimensions array][raw data]**

The sample python file to extract the dataset is located [here](/ExternalTools/BinaryReader.py)

A good baseline for how to train your model will most definitely be the sample model training files from Daniel Holden or Sebastian Starke papers.
Specifically the MotionMatching repository by TheOrangeDuck. 

IMPORTANT
Make sure the model includes normalisation and denormalisation layers at each end. You can try to parse the std and mean through binaries but this approach is way easier.

### Run the model in real time

Running the animation in real time can be done through the Unreal Animation Blueprint system through **Neural Network** AnimNode included in the plugin. It serves as a starting point in defining your real time approach, with loading the features, computing the global/local positions and applying smoothing through inertialisation (although it is fairly slow. My bad!).

The anim graph should look something like this

![AnimGraph](/Media/AnimNode.png)

1. Neural Network node from the plugin
2. Cached Current pose. Done through pose snapshot. If this is your approach make sure to set this set of nodes up in the event graph

![EventGraph](/Media/EventGraph.png)

3. Select your chosen feature set and the trained onnx model from the asset registry

4. Properties related to inertialisation and other stuff related to the formatting

Please note that the animnode in the project simply serves as a starting point and it is not a sample demo with a working model. Thats your job :)

## Creating custom features

The plugin can be extended with additional feature classes to better fit the need of the developer. The new feature should contain all the functionality for outputting feature vector both offline and in real time.

### Define new Feature
The project contains sample features for trajectory and bone information, as seen in Starke and Holden papers on procedural animation. To create a custom feature, simply inherit from the **UFeature** class and override the necessary functions as shown in the **UBoneFeature** and **UTrajectoryFeature** examples. 

```

// Either in youre feature file or just Features.h

UCLASS()
class NEURALANIMATIONTOOLKIT_API UCustomFeature : public UFeature
{
	GENERATED_BODY()

public:

	UCustomFeature() = default;

	// FeatureComputeInterface
	void InitialiseOffline(const FReferenceSkeleton& RefSkeleton) override { *** }
	void InitialiseRealTime(const FBoneContainer& BoneContainer) override 
    { 
        ***
	TArray<float> ComputeRealTime(const FBoneContainer& BoneContainer, FCSPose<FCompactPose> InPose, float DeltaTime) override
	{
		***
	}

	TArray<float> ComputeOffline(const TArray<TArray<FTransform>>& BoneTransforms, float DeltaTime, int FrameIndex) override
	{
		***
	}

	int32 GetFeatureSize() const override
	{
        ***
	}
	// End FeatureComputeInterface
};

```

Each feature should have implemented versions of offline and realtmie computation that return a float array representing a feature vector. The class also exposes the initialisation functions in case of getting an appropriate bone indexes for the bone references or iniialising them in animnode.

### Expose to Feature Builder

The second step is to expose the new feature to the **FeatureBuilder** widget as a button. In **UFeatureBuilder** class, add a new **UEditorUtilityButton** pointer as well as the corresponding *OnNewCustomFeatureButtonClicked* function to register to the new button. 


```
UCLASS(Abstract)
class NEURALANIMATIONTOOLKIT_API UFeatureSetBuilder : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:

    ***

    // Add this
    UPROPERTY(EditAnywhere, meta = (BindWidget))
    TObjectPtr<UEditorUtilityButton> NewCustomFeatureButton;

private:

    ***

    // And this
    UFUNCTION()
    void OnNewCustomFeatureButtonClicked(); // Button to create a new trajectory feature

};
```

In the cpp file, append the NativeConstruct function and then simply copy paste the code from other button function and replace it with your feature class


```
// In FeatureSetBuilder.cpp

void UFeatureSetBuilder::NativeConstruct()
{
	Super::NativeConstruct();

    ***

	if (NewCustomFeatureButton) {
		NewCustomFeatureButton->OnClicked.AddDynamic(this, &UFeatureSetBuilder::OnNewCustomFeatureButtonClicked);
	}

    ***

}

***

void UFeatureSetBuilder::OnNewCustomFeatureButtonClicked()
{
	if (!FeatureSetSchema)
	{
		return;
	}

	UCustomFeature* NewCustomFeature = NewObject<UCustomFeature>(FeatureSetSchema);
	FeatureSetSchema->AddFeature(NewCustomFeature);
	FeatureSetSchema->MarkPackageDirty();
	FeatureSetSchema->PostEditChange();

	LoadFeatureList();

}

```

### Add a Custom Feature Button in UMG
After the button has been defined in the code, the visual side needs to be added. Simply open the FeatureBuilder Widget in UMG and add a corresponding New Custom Feature Button. Make sure the name of the new button matches that of the pointer in the .h file.

![CustomFeatureButton](Media/CustomFeature.png)

### TODO
* Add sequences start/end indexes to the exported files 
* Package the plugin with the tools provided by UE5 Build System. Right now the only way to include it in the project is to drop the files in
* Provide sample implementation, maybe some common models (PFNN, Codematching, LMM)