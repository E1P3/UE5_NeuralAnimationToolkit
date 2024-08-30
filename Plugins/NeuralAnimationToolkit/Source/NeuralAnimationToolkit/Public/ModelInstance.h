#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "NNE.h"
#include "NNERuntimeCPU.h"
#include "NNEModelData.h"
#include "ModelInstance.generated.h"

USTRUCT(BlueprintType, Category = "Neural Network")
struct FNeuralNetworkTensor
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, Category = "Neural Network")
	TArray<int32> Shape = TArray<int32>();

	UPROPERTY(BlueprintReadWrite, Category = "Neural Network")
	TArray<float> Data = TArray<float>(); // this is a flat array, assume it gets reshaped to match Shape in a smart way like in Python
	//TArray<TArray<float>> Data = TArray<TArray<float>>();
};

struct FModelInstance {

	TUniquePtr<UE::NNE::IModelInstanceCPU> ModelInstance;
	TArray<float> InputData;
	TArray<float> OutputData;
	TArray<UE::NNE::FTensorBindingCPU> InputBindings;
	TArray<UE::NNE::FTensorBindingCPU> OutputBindings;
	TArray<UE::NNE::FTensorShape> InputTensorShapes;
	TArray<UE::NNE::FTensorShape> OutputTensorShapes;
	bool bIsRunning = false;
	bool bIsFinished = false;

	FModelInstance() = default;
	FModelInstance(const TObjectPtr<UNNEModelData> ModelData, const TWeakInterfacePtr<INNERuntimeCPU> Runtime);

	void SetInputData(TArray<float> Data);
	void ClearOutputData();
	void Initialize(const TObjectPtr<UNNEModelData> ModelData, const TWeakInterfacePtr<INNERuntimeCPU> Runtime);

	int RunModel(TArray<float> _InputData);
	static bool CreateTensor(TArray<int32> Shape, UPARAM(ref) FNeuralNetworkTensor& Tensor);

	int32 NumInputs() const;
	int32 NumOutputs() const;
	TArray<int32> GetInputShape(int32 Index) const;
	TArray<int32> GetOutputShape(int32 Index) const;
};
