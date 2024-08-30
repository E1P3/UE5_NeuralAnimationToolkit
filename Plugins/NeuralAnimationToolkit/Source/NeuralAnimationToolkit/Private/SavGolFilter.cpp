#include "SavGolFilter.h"
#include "Algo/Reverse.h"
#include "Math/UnrealMathUtility.h"

// HandleBoundary remains unchanged
int32 USavGolFilter::HandleBoundary(int32 Index, int32 Length, const FString& Mode)
{
        if (Mode == "reflect")
        {
                if (Index < 0)
                {
                        return -Index;
                }
                else if (Index >= Length)
                {
                        return 2 * Length - Index - 2;
                }
        }
        else if (Mode == "constant")
        {
                if (Index < 0 || Index >= Length)
                {
                        return -1; // Indicate out-of-bound for constant mode
                }
        }
        return Index;
}

TArray<FVector> USavGolFilter::Correlate1D(const TArray<FVector>& Input, const TArray<float>& Weights, int32 Axis, const FString& Mode, const FVector& CVal, int32 Origin)
{
        int32 InputLength = Input.Num();
        int32 WeightsLength = Weights.Num();

        if (WeightsLength <= 0)
        {
                UE_LOG(LogTemp, Error, TEXT("No filter weights given."));
                return TArray<FVector>();
        }

        if (Origin < -(WeightsLength / 2) || Origin >((WeightsLength - 1) / 2))
        {
                UE_LOG(LogTemp, Error, TEXT("Invalid origin; origin must satisfy -(len(weights) // 2) <= origin <= (len(weights)-1) // 2"));
                return TArray<FVector>();
        }

        TArray<FVector> Output;
        Output.SetNumZeroed(InputLength);

        for (int32 i = 0; i < InputLength; ++i)
        {
                FVector CorrelatedValue = FVector::ZeroVector;
                for (int32 j = 0; j < WeightsLength; ++j)
                {
                        int32 InputIndex = i + j + Origin;
                        InputIndex = HandleBoundary(InputIndex, InputLength, Mode);

                        if (InputIndex >= 0 && InputIndex < InputLength)
                        {
                                CorrelatedValue += Input[InputIndex] * Weights[j];
                        }
                        else if (Mode == "constant")
                        {
                                CorrelatedValue += CVal * Weights[j]; // Apply Cval for out-of-bound indices in constant mode
                        }
                }
                Output[i] = CorrelatedValue;
        }

        return Output;
}

TArray<FVector> USavGolFilter::Convolve1D(const TArray<FVector>& Input, TArray<float>& Weights, int32 Axis, const FString& Mode, const FVector& CVal, int32 Origin)
{
        Algo::Reverse(Weights);

        Origin = -Origin;
        if (Weights.Num() % 2 == 0)
        {
                Origin -= 1;
        }

        return Correlate1D(Input, Weights, Axis, Mode, CVal, Origin);
}

TArray<FVector> USavGolFilter::SavGolFilter(const TArray<FVector>& X, int32 WindowLength, int32 PolyOrder, int32 Deriv, float Delta, int32 Axis, const FString& Mode, const FVector& Cval)
{
        if (WindowLength % 2 == 0 || WindowLength <= 0 || PolyOrder < 0 || WindowLength <= PolyOrder)
        {
                return TArray<FVector>();
        }

        TArray<float> Coeffs = SavGolCoeffs(WindowLength, PolyOrder, Deriv, Delta);

        TArray<FVector> Y;
        Y.SetNum(X.Num());

        if (Mode == "interp")
        {
                if (WindowLength > X.Num())
                {
                        return TArray<FVector>();
                }

                Y = Convolve1D(X, Coeffs, Axis, "constant", Cval);
                FitEdgesPolyFit(X, WindowLength, PolyOrder, Deriv, Delta, Y);
        }
        else
        {
                Y = Convolve1D(X, Coeffs, Axis, Mode, Cval);
        }

        return Y;
}

// Other functions (SavGolCoeffs, FitEdgesPolyFit, FitEdge, PolyDer) remain largely the same,
// except FitEdge and FitEdgesPolyFit should handle FVector operations.

void USavGolFilter::FitEdgesPolyFit(const TArray<FVector>& X, int32 WindowLength, int32 PolyOrder, int32 Deriv, float Delta, TArray<FVector>& Y)
{
        int32 HalfLen = WindowLength / 2;
        FitEdge(X, 0, WindowLength, 0, HalfLen, -1, PolyOrder, Deriv, Delta, Y);
        FitEdge(X, X.Num() - WindowLength, X.Num(), X.Num() - HalfLen, X.Num(), -1, PolyOrder, Deriv, Delta, Y);
}

void USavGolFilter::FitEdge(const TArray<FVector>& X, int32 WindowStart, int32 WindowStop, int32 InterpStart, int32 InterpStop, int32 Axis, int32 PolyOrder, int32 Deriv, float Delta, TArray<FVector>& Y)
{
        TArray<FVector> XEdge = Slice(X, WindowStart, WindowStop - WindowStart);

        TArray<float> PolyCoeffs;
        PolyCoeffs.SetNum(PolyOrder + 1);

        for (int32 i = 0; i <= PolyOrder; ++i)
        {
                FVector Sum = FVector::ZeroVector;
                for (int32 j = 0; j < XEdge.Num(); ++j)
                {
                        Sum += XEdge[j] * FMath::Pow(static_cast<float>(j), static_cast<float>(i));
                }
                PolyCoeffs[i] = Sum.Size(); // Assuming we're summing the size of vectors; adapt as needed
        }

        if (Deriv > 0)
        {
                PolyCoeffs = PolyDer(PolyCoeffs, Deriv);
        }

        TArray<FVector> Values;
        for (int32 i = InterpStart - WindowStart; i < InterpStop - WindowStart; ++i)
        {
                FVector Val = FVector::ZeroVector;
                for (int32 j = 0; j < PolyCoeffs.Num(); ++j)
                {
                        Val += FVector(PolyCoeffs[j]) * FMath::Pow(static_cast<float>(i), static_cast<float>(j));
                }
                Values.Add(Val / FMath::Pow(Delta, static_cast<float>(Deriv)));
        }

        for (int32 i = InterpStart; i < InterpStop; ++i)
        {
                Y[i] = Values[i - InterpStart];
        }
}

TArray<FVector> USavGolFilter::Slice(const TArray<FVector>& array, int32 start, int32 stop)
{
        TArray<FVector> result;
        for (int32 i = start; i < stop; i++)
        {
                result.Add(array[i]);
        }
        return result;
}

// PolyDer and SavGolCoeffs remain the same

TArray<float> USavGolFilter::SavGolCoeffs(int32 WindowLength, int32 PolyOrder, int32 Deriv, float Delta, int32 Pos, const FString& Use)
{
    if (PolyOrder >= WindowLength)
    {
        // Handle error
        return TArray<float>();
    }

    int32 HalfLen = WindowLength / 2;
    if (Pos == -1)
    {
        Pos = (WindowLength % 2 == 0) ? HalfLen - 1 : HalfLen;
    }

    TArray<float> X;
    for (int32 i = -Pos; i < WindowLength - Pos; ++i)
    {
        X.Add(i);
    }

    if (Use == "conv")
    {
        Algo::Reverse(X);
    }

    TArray<float> Y;
    Y.SetNumZeroed(PolyOrder + 1);
    Y[Deriv] = FMath::Pow(Delta, -Deriv);

    TArray<float> Coeffs;
    Coeffs.SetNum(WindowLength);

    for (int32 i = 0; i < WindowLength; ++i)
    {
        float Sum = 0.0f;
        for (int32 j = 0; j <= PolyOrder; ++j)
        {
            float Term = 1.0f;
            for (int32 k = 0; k < j; ++k)
            {
                Term *= X[i];
            }
            Sum += Term * Y[j];
        }
        Coeffs[i] = Sum;
    }

    return Coeffs;
}

TArray<float> USavGolFilter::PolyDer(const TArray<float>& P, int32 M)
{
    TArray<float> Result = P;

    for (int32 m = 0; m < M; ++m)
    {
        for (int32 i = 0; i < Result.Num() - 1; ++i)
        {
            Result[i] = Result[i] * (Result.Num() - 1 - i);
        }
        Result.Pop();
    }

    return Result;
}
