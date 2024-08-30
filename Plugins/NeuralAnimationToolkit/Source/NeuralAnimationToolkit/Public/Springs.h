#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "FeatureComputation.h"
#include "Springs.generated.h"

USTRUCT(BlueprintType)
struct NEURALANIMATIONTOOLKIT_API FSpring {

	GENERATED_BODY()

	float x = 0.0f;
	float v = 0.0f;

	float x_gain = 0.01f;
	float v_gain = 0.2f;
	float a_gain = 1.0f;

	float v_max = 500.0f;
	float a_max = 500.0f;

	float* g_prev;
	float prev_size = 3;

	FSpring(int prev_size = 3, float x_gain = 0.01f, float v_gain = 0.2f, float a_gain = 0.1f);

	void update(float g, const float dt);

private:
	void tracking_spring_update_exact(
		float x_goal,
		float v_goal,
		float a_goal,
		const float dt,
		float gain_dt);

	void tracking_spring_update_no_acceleration_exact(
		float x_goal,
		float v_goal,
		const float dt,
		float gain_dt);

	void tracking_spring_update_no_velocity_acceleration_exact(
		float x_goal,
		const float dt,
		float gain_dt);

	float tracking_target_acceleration(
		float x_next,
		float x_curr,
		float x_prev,
		const float dt);

	float tracking_target_velocity(
		float x_next,
		float x_curr,
		const float dt);

	void spring_damper_exact_stiffness_damping(
		float x_goal,
		float v_goal,
		float stiffness,
		float damping,
		const float dt,
		float eps = 1e-5f);

};

USTRUCT(BlueprintType)
struct NEURALANIMATIONTOOLKIT_API FVectorSpring
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spring")
	float halfLife = 0.1f;

	FVectorSpring(float HalfLife = 0.1f);

	void Update(FVector& g, const float dt);

private:
	FVector x;
	FVector v;
	float y;
};

USTRUCT(BlueprintType)
struct NEURALANIMATIONTOOLKIT_API FQuatSpring
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spring")
	float halfLife = 0.1f;

	FQuatSpring(float HalfLife = 0.1f);

	void Update(FQuat& g, const float dt);

private:
	FQuat q;
	FVector v;
	float y;
};

USTRUCT(BlueprintType)
struct NEURALANIMATIONTOOLKIT_API FTransformSpring
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spring")
	float halfLife = 0.1f;

	FTransformSpring(float HalfLife = 0.1f);

	void Update(FTransform& g, const float dt, bool isDebug = false);

private:
	FVectorSpring positionSpring;
	FQuatSpring rotationSpring;
};

UCLASS(Abstract)
class NEURALANIMATIONTOOLKIT_API UQuatHelper : public UObject
{
	GENERATED_BODY()

public:
	static FQuat QuatExp(const FVector& V, float Eps);
	static FVector QuatLog(const FQuat& Q, float Eps);
	static FQuat QuatFromScaledAngleAxis(const FVector& V, float Eps = 1e-8f);
	static FVector QuatToScaledAngleAxis(const FQuat& Q, float Eps = 1e-8f);
};
