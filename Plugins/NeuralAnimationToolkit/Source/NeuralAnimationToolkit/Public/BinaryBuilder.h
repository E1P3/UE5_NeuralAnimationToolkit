#pragma once

#include "CoreMinimal.h"
#include "Misc/FileHelper.h"
#include "Serialization/BufferArchive.h"
#include "Misc/Paths.h"

// Simple binary parser
// The format of the binary file is as follows:
// 1. Dimension Array Size
// 2. Dimension Array
// 3. Data Array
//
// The dimension array is an array of integers that represent the dimensions of the data array
// The data array is an array of floats that represent the data passed down to the function

// The binary can be then read in by sample python script (ExternalTools/BinaryReader.py) to then be used in the training of the neural network
class NEURALANIMATIONTOOLKIT_API UBinaryBuilder
{
public:
    static bool SaveToBinaryFile(const FString& FilePath, const TArray<int32>& Dimensions, const TArray<float>& Data);
    static bool SaveToBinaryFile(const FString& FilePath, const TArray<int32>& Dimensions, const TArray<int32>& Data);
    static TArray<float> LoadFromBinaryFile(const FString& FilePath);
};
