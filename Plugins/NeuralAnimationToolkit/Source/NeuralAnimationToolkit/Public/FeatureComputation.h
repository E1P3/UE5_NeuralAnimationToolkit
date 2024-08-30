#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "FeatureComputation.generated.h"

// Helper function for all bone related computations
UCLASS(Abstract)
class NEURALANIMATIONTOOLKIT_API UFeatureComputation : public UObject
{
	GENERATED_BODY()

public:

	static int32 GetBoneIndex(const FReferenceSkeleton RefSkeleton, const FBoneReference Bone) {
		int32 CurrentBoneIndex = RefSkeleton.FindBoneIndex(Bone.BoneName);
		return CurrentBoneIndex;
	}

	static FVector GetBoneVelocity(const FTransform& PreviousTransform, const FTransform& CurrentTransform, const FTransform& NextTransform, const float DeltaTime)
	{
		FVector Velocity = FVector::ZeroVector;
		if (DeltaTime > 0.0f) {
			Velocity = 0.5f * (CurrentTransform.GetLocation() - PreviousTransform.GetLocation()) / DeltaTime + 0.5f * (NextTransform.GetLocation() - CurrentTransform.GetLocation()) / DeltaTime;
		}
		return Velocity;
	}

	static FVector GetBoneAngularVelocity(const FTransform& PreviousTransform, const FTransform& CurrentTransform, const FTransform& NextTransform, const float DeltaTime)
	{
		FVector AngularVelocity = FVector::ZeroVector;

		if (DeltaTime > 0.0f) {
			FQuat PrevRotation = PreviousTransform.GetRotation();
			FQuat CurrRotation = CurrentTransform.GetRotation();
			FQuat NextRotation = NextTransform.GetRotation();

			FQuat DeltaRotNext = NextRotation * CurrRotation.Inverse();
			FQuat DeltaRotPrev = CurrRotation * PrevRotation.Inverse();

			DeltaRotNext.Normalize();
			DeltaRotPrev.Normalize();

			FVector AngularVelNext = QuatToScaledAngleAxis(DeltaRotNext);
			FVector AngularVelPrev = QuatToScaledAngleAxis(DeltaRotPrev);

			AngularVelocity = 0.5f * (AngularVelNext + AngularVelPrev) * (1.0f / DeltaTime);
		}

		return AngularVelocity;
	}


	static FQuat QuatExp(const FVector& V, float Eps = 1e-8f)
	{
		float HalfAngle = FMath::Sqrt(V.X * V.X + V.Y * V.Y + V.Z * V.Z);

		if (HalfAngle < Eps)
		{
			return FQuat::MakeFromEuler(V).GetNormalized();
		}
		else
		{
			float C = FMath::Cos(HalfAngle);
			float S = FMath::Sin(HalfAngle) / HalfAngle;
			return FQuat(S * V.X, S * V.Y, S * V.Z, C).GetNormalized();
		}
	}

	static FVector QuatLog(const FQuat& Q, float Eps = 1e-8f)
	{
		float Length = FMath::Sqrt(Q.X * Q.X + Q.Y * Q.Y + Q.Z * Q.Z);

		if (Length < Eps)
		{
			return FVector(Q.X, Q.Y, Q.Z);
		}
		else
		{
			float HalfAngle = FMath::Acos(FMath::Clamp(Q.W, -1.0f, 1.0f));
			return (HalfAngle / Length) * FVector(Q.X, Q.Y, Q.Z);
		}
	}

	static FQuat QuatFromScaledAngleAxis(const FVector& V, float Eps = 1e-8f)
	{
		return QuatExp(V / 2.0f, Eps);
	}

	static FVector QuatToScaledAngleAxis(const FQuat& Q, float Eps = 1e-8f)
	{
		return 2.0f * QuatLog(Q, Eps);
	}

	static FQuat GetQuatFromXformXY(FVector x, FVector y)
	{
		FVector c2 = FVector::CrossProduct(x, y);
		FVector c1 = FVector::CrossProduct(c2, x);
		FVector c0 = x;


		FQuat result = GetQuatFromColumns(c0, c1, c2);
		result.Normalize();
		return result;
	}

	static void GetXformXYFromQuat(FQuat Q, FVector& x, FVector& y) {
		FVector c0, c1, c2;
		GetColumnsFromQuat(Q, c0, c1, c2);

		x = c0;  // x is directly c0
		y = c1;  // y is directly c1
	}

	static FQuat GetQuatFromColumns(FVector c0, FVector c1, FVector c2) {
		if (c2.Z < 0.0f)
		{
			if (c0.X > c1.Y)
			{
				return FQuat(
					1.0f + c0.X - c1.Y - c2.Z,
					c0.Y + c1.X,
					c2.X + c0.Z,
					c1.Z - c2.Y);
			}
			else
			{
				return FQuat(
					c0.Y + c1.X,
					1.0f - c0.X + c1.Y - c2.Z,
					c1.Z + c2.Y,
					c2.X - c0.Z);
			}
		}
		else
		{
			if (c0.X < -c1.Y)
			{
				return FQuat(
					c2.X + c0.Z,
					c1.Z + c2.Y,
					1.0f - c0.X - c1.Y + c2.Z,
					c0.Y - c1.X);
			}
			else
			{
				return FQuat(
					c1.Z - c2.Y,
					c2.X - c0.Z,
					c0.Y - c1.X,
					1.0f + c0.X + c1.Y + c2.Z);
			}
		}

	}

	static void GetColumnsFromQuat(FQuat Q, FVector& c0, FVector& c1, FVector& c2) {
		float qx = Q.X;
		float qy = Q.Y;
		float qz = Q.Z;
		float qw = Q.W;

		c0 = FVector(
			1.0f - 2.0f * (qy * qy + qz * qz),
			2.0f * (qx * qy + qw * qz),
			2.0f * (qx * qz - qw * qy)
		);

		c1 = FVector(
			2.0f * (qx * qy - qw * qz),
			1.0f - 2.0f * (qx * qx + qz * qz),
			2.0f * (qy * qz + qw * qx)
		);

		c2 = FVector(
			2.0f * (qx * qz + qw * qy),
			2.0f * (qy * qz - qw * qx),
			1.0f - 2.0f * (qx * qx + qy * qy)
		);
	}

};
