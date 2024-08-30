#include "BinaryBuilder.h"

bool UBinaryBuilder::SaveToBinaryFile(const FString& FilePath, const TArray<int32>& Dimensions, const TArray<float>& Data)
{
        FBufferArchive Buffer;

        int32 DimensionCount = Dimensions.Num();
        Buffer << DimensionCount;

        for (int32 Dimension : Dimensions)
        {
                Buffer << Dimension;  // Serialize eac
        }

        // Serialize the float data
        for (float Value : Data)
        {
                Buffer << Value;
        }

        TArray<uint8> RawData;
        RawData.Append(Buffer.GetData(), Buffer.Num());

        return FFileHelper::SaveArrayToFile(RawData, *FPaths::Combine(FPaths::ProjectDir(), FilePath));
}

bool UBinaryBuilder::SaveToBinaryFile(const FString& FilePath, const TArray<int32>& Dimensions, const TArray<int32>& Data)
{
        FBufferArchive Buffer;

        int32 DimensionCount = Dimensions.Num();
        Buffer << DimensionCount;

        for (int32 Dimension : Dimensions)
        {
                Buffer << Dimension;  // Serialize eac
        }

        // Serialize the float data
        for (int32 Value : Data)
        {
                Buffer << Value;
        }

        TArray<uint8> RawData;
        RawData.Append(Buffer.GetData(), Buffer.Num());

        return FFileHelper::SaveArrayToFile(RawData, *FPaths::Combine(FPaths::ProjectDir(), FilePath));
}

TArray<float> UBinaryBuilder::LoadFromBinaryFile(const FString& FilePath)
{
    TArray<uint8> RawData;
    FFileHelper::LoadFileToArray(RawData, *FilePath);

    TArray<float> Data;
    Data.SetNum(RawData.Num() / sizeof(float));
    FMemory::Memcpy(Data.GetData(), RawData.GetData(), RawData.Num());

    return Data;
}