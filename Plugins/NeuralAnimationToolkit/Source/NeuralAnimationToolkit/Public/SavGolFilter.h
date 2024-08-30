#pragma once

#include "CoreMinimal.h"
#include "Kismet/KismetMathLibrary.h"

class USavGolFilter
{
public:
	static TArray<FVector> SavGolFilter(const TArray<FVector>& X, int32 WindowLength, int32 PolyOrder, int32 Deriv = 0, float Delta = 1.0f, int32 Axis = -1, const FString& Mode = "interp", const FVector& Cval = FVector::ZeroVector);

private:
	static int32 HandleBoundary(int32 Index, int32 Length, const FString& Mode);
	static TArray<FVector> Correlate1D(const TArray<FVector>& Input, const TArray<float>& Weights, int32 Axis = -1, const FString& Mode = "reflect", const FVector& CVal = FVector::ZeroVector, int32 Origin = 0);
	static TArray<FVector> Convolve1D(const TArray<FVector>& Input, TArray<float>& Weights, int32 Axis = -1, const FString& Mode = "reflect", const FVector& CVal = FVector::ZeroVector, int32 Origin = 0);
	static TArray<FVector> Slice(const TArray<FVector>& array, int32 start, int32 stop);
	static TArray<float> SavGolCoeffs(int32 WindowLength, int32 PolyOrder, int32 Deriv = 0, float Delta = 1.0f, int32 Pos = -1, const FString& Use = "conv");
	static void FitEdgesPolyFit(const TArray<FVector>& X, int32 WindowLength, int32 PolyOrder, int32 Deriv, float Delta, TArray<FVector>& Y);
	static void FitEdge(const TArray<FVector>& X, int32 WindowStart, int32 WindowStop, int32 InterpStart, int32 InterpStop, int32 Axis, int32 PolyOrder, int32 Deriv, float Delta, TArray<FVector>& Y);
	static TArray<float> PolyDer(const TArray<float>& P, int32 M);
};
