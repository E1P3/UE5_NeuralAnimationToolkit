#include "ModelInstance.h"

FModelInstance::FModelInstance(const TObjectPtr<UNNEModelData> ModelData, const TWeakInterfacePtr<INNERuntimeCPU> Runtime) {
        TUniquePtr<UE::NNE::IModelCPU> Model = Runtime->CreateModel(ModelData);
        if (Model.IsValid()) {
                ModelInstance = Model->CreateModelInstance();
                if (ModelInstance.IsValid()) {
                        TConstArrayView<UE::NNE::FTensorDesc> InputTensorDescs = ModelInstance->GetInputTensorDescs();
                        UE::NNE::FSymbolicTensorShape SymbolicInputTensorShape = InputTensorDescs[0].GetShape();
                        InputTensorShapes = { UE::NNE::FTensorShape::MakeFromSymbolic(SymbolicInputTensorShape) };

                        ModelInstance->SetInputTensorShapes(InputTensorShapes);

                        TConstArrayView<UE::NNE::FTensorDesc> OutputTensorDescs = ModelInstance->GetOutputTensorDescs();
                        UE::NNE::FSymbolicTensorShape SymbolicOutputTensorShape = OutputTensorDescs[0].GetShape();
                        OutputTensorShapes = { UE::NNE::FTensorShape::MakeFromSymbolic(SymbolicOutputTensorShape) };

                        // Example for creating in- and outputs
                        InputData.SetNumZeroed(InputTensorShapes[0].Volume());
                        InputBindings.SetNumZeroed(1);
                        InputBindings[0].Data = InputData.GetData();
                        InputBindings[0].SizeInBytes = InputData.Num() * sizeof(float);

                        OutputData.SetNumZeroed(OutputTensorShapes[0].Volume());
                        OutputBindings.SetNumZeroed(1);
                        OutputBindings[0].Data = OutputData.GetData();
                        OutputBindings[0].SizeInBytes = OutputData.Num() * sizeof(float);

                        UE_LOG(LogTemp, Warning, TEXT("Created Model with %d inputs and %d outputs"), InputTensorShapes[0].Volume(), OutputTensorShapes[0].Volume());
                }
        }
}

void FModelInstance::SetInputData(TArray<float> Data) {
        if (InputData.Num() != InputTensorShapes[0].Volume()) {
                UE_LOG(LogTemp, Error, TEXT("Input data size does not match model input size (%d != %d)"), InputData.Num(), InputTensorShapes[0].Volume());
                return;
        }

        InputData = Data;
        InputBindings[0].Data = InputData.GetData();
        InputBindings[0].SizeInBytes = InputData.Num() * sizeof(float);
}

void FModelInstance::ClearOutputData() {
        OutputData.Empty();
        OutputData.SetNumZeroed(OutputTensorShapes[0].Volume());
        OutputBindings[0].Data = OutputData.GetData();
        OutputBindings[0].SizeInBytes = OutputData.Num() * sizeof(float);
}

void FModelInstance::Initialize(const TObjectPtr<UNNEModelData> ModelData, const TWeakInterfacePtr<INNERuntimeCPU> Runtime){
        TUniquePtr<UE::NNE::IModelCPU> Model = Runtime->CreateModel(ModelData);
        if (Model.IsValid()) {
                ModelInstance = Model->CreateModelInstance();
                if (ModelInstance.IsValid()) {
                        TConstArrayView<UE::NNE::FTensorDesc> InputTensorDescs = ModelInstance->GetInputTensorDescs();
                        UE::NNE::FSymbolicTensorShape SymbolicInputTensorShape = InputTensorDescs[0].GetShape();
                        InputTensorShapes = { UE::NNE::FTensorShape::MakeFromSymbolic(SymbolicInputTensorShape) };

                        ModelInstance->SetInputTensorShapes(InputTensorShapes);

                        TConstArrayView<UE::NNE::FTensorDesc> OutputTensorDescs = ModelInstance->GetOutputTensorDescs();
                        UE::NNE::FSymbolicTensorShape SymbolicOutputTensorShape = OutputTensorDescs[0].GetShape();
                        OutputTensorShapes = { UE::NNE::FTensorShape::MakeFromSymbolic(SymbolicOutputTensorShape) };

                        // Example for creating in- and outputs
                        InputData.SetNumZeroed(InputTensorShapes[0].Volume());
                        InputBindings.SetNumZeroed(1);
                        InputBindings[0].Data = InputData.GetData();
                        InputBindings[0].SizeInBytes = InputData.Num() * sizeof(float);

                        OutputData.SetNumZeroed(OutputTensorShapes[0].Volume());
                        OutputBindings.SetNumZeroed(1);
                        OutputBindings[0].Data = OutputData.GetData();
                        OutputBindings[0].SizeInBytes = OutputData.Num() * sizeof(float);

                        UE_LOG(LogTemp, Warning, TEXT("Created Model with %d inputs and %d outputs"), InputTensorShapes[0].Volume(), OutputTensorShapes[0].Volume());
                }
        }
}

int FModelInstance::RunModel(TArray<float> _InputData) {
        SetInputData(_InputData);
        ClearOutputData();

        if (ModelInstance->RunSync(InputBindings, OutputBindings) != 0) {
                UE_LOG(LogTemp, Error, TEXT("ModelInstance: Failed to run the model"));
                return 0;
        }

        return 1;
}

bool FModelInstance::CreateTensor(TArray<int32> Shape, UPARAM(ref) FNeuralNetworkTensor& Tensor) {
        if (Shape.Num() == 0) {
                return false;
        }

        int32 Volume = 1;
        for (int32 i = 0; i < Shape.Num(); i++) {
                if (Shape[i] < 1) {
                        return false;
                }
                Volume *= Shape[i];
        }

        Tensor.Shape = Shape;
        Tensor.Data.SetNum(Volume);
        return true;
}

int32 FModelInstance::NumInputs() const {
        return InputTensorShapes[0].Rank();
}

int32 FModelInstance::NumOutputs() const {
        return OutputTensorShapes[0].Rank();
}

TArray<int32> FModelInstance::GetInputShape(int32 Index) const {
        check(ModelInstance.IsValid());

        using namespace UE::NNE;

        TConstArrayView<FTensorDesc> Desc = ModelInstance->GetInputTensorDescs();
        if (Index < 0 || Index >= Desc.Num()) {
                return TArray<int32>();
        }

        return TArray<int32>(Desc[Index].GetShape().GetData());
}

TArray<int32> FModelInstance::GetOutputShape(int32 Index) const {
        check(ModelInstance.IsValid());

        using namespace UE::NNE;

        TConstArrayView<FTensorDesc> Desc = ModelInstance->GetOutputTensorDescs();
        if (Index < 0 || Index >= Desc.Num()) {
                return TArray<int32>();
        }

        return TArray<int32>(Desc[Index].GetShape().GetData());
}
