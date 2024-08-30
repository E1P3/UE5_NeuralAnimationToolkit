#include "Springs.h"

FSpring::FSpring(int prev_size, float x_gain, float v_gain, float a_gain)
{
        this->x = x;
        this->v = v;
        this->x_gain = x_gain;
        this->v_gain = v_gain;
        this->a_gain = a_gain;

        this->prev_size = prev_size;
        this->g_prev = new float[prev_size];
}

void FSpring::update(float g, const float dt)
{
        for (int i = prev_size - 1; i > 0; i--)
        {
                g_prev[i] = g_prev[i - 1];
        }

        float x_goal = g;
        float v_goal = FMath::Clamp(tracking_target_velocity(g, g_prev[1], dt), -v_max, v_max);
        float a_goal = FMath::Clamp(tracking_target_acceleration(g, g_prev[1], g_prev[2], dt), -a_max, a_max);

        tracking_spring_update_exact(
                x_goal,
                v_goal,
                a_goal,
                dt,
                dt);

        g_prev[0] = g;

}

void FSpring::tracking_spring_update_exact(
        float x_goal,
        float v_goal,
        float a_goal,
        const float dt,
        float gain_dt)
{
        float t0 = (1.0f - v_gain) * (1.0f - x_gain);
        float t1 = a_gain * (1.0f - v_gain) * (1.0f - x_gain);
        float t2 = (v_gain * (1.0f - x_gain)) / gain_dt;
        float t3 = x_gain / (gain_dt * gain_dt);

        float stiffness = t3;
        float damping = (1.0f - t0) / gain_dt;
        float spring_x_goal = x_goal;
        float spring_v_goal = (t2 * v_goal + t1 * a_goal) / ((1.0f - t0) / gain_dt);

        spring_damper_exact_stiffness_damping(
                spring_x_goal,
                spring_v_goal,
                stiffness,
                damping,
                dt);
}

void FSpring::tracking_spring_update_no_acceleration_exact(
        float x_goal,
        float v_goal,
        const float dt,
        float gain_dt)
{
        float t0 = (1.0f - v_gain) * (1.0f - x_gain);
        float t2 = (v_gain * (1.0f - x_gain)) / gain_dt;
        float t3 = x_gain / (gain_dt * gain_dt);

        float stiffness = t3;
        float damping = (1.0f - t0) / gain_dt;
        float spring_x_goal = x_goal;
        float spring_v_goal = t2 * v_goal / ((1.0f - t0) / gain_dt);

        spring_damper_exact_stiffness_damping(
                spring_x_goal,
                spring_v_goal,
                stiffness,
                damping,
                dt);
}

void FSpring::tracking_spring_update_no_velocity_acceleration_exact(
        float x_goal,
        const float dt,
        float gain_dt)
{
        float t0 = 1.0f - x_gain;
        float t3 = x_gain / (gain_dt * gain_dt);

        float stiffness = t3;
        float damping = (1.0f - t0) / gain_dt;
        float spring_x_goal = x_goal;
        float spring_v_goal = 0.0f;

        spring_damper_exact_stiffness_damping(
                spring_x_goal,
                spring_v_goal,
                stiffness,
                damping,
                dt);
}

float FSpring::tracking_target_acceleration(
        float x_next,
        float x_curr,
        float x_prev,
        const float dt)
{
        return (((x_next - x_curr) / dt) - ((x_curr - x_prev) / dt)) / dt;
}

float FSpring::tracking_target_velocity(
        float x_next,
        float x_curr,
        const float dt)
{
        return (x_next - x_curr) / dt;
}

void FSpring::spring_damper_exact_stiffness_damping(
        float x_goal,
        float v_goal,
        float stiffness,
        float damping,
        const float dt,
        float eps)
{
        float g = x_goal;
        float q = v_goal;
        float s = stiffness;
        float d = damping;
        float c = g + (d * q) / (s + eps);
        float y = d / 2.0f;

        if (FMath::Abs(s - (d * d) / 4.0f) < eps) // Critically Damped
        {
                float j0 = x - c;
                float j1 = v + j0 * y;

                float eydt = FMath::Exp(-y * dt);

                x = j0 * eydt + dt * j1 * eydt + c;
                v = -y * j0 * eydt - y * dt * j1 * eydt + j1 * eydt;
        }
        else if (s - (d * d) / 4.0f > 0.0) // Under Damped
        {
                float w = FMath::Sqrt(s - (d * d) / 4.0f);
                float j = FMath::Sqrt(FMath::Square(v + y * (x - c)) / (w * w + eps) + FMath::Square(x - c));
                float p = FMath::Atan((v + (x - c) * y) / (-(x - c) * w + eps));

                j = (x - c) > 0.0f ? j : -j;

                float eydt = FMath::Exp(-y * dt);

                x = j * eydt * FMath::Cos(w * dt + p) + c;
                v = -y * j * eydt * FMath::Cos(w * dt + p) - w * j * eydt * FMath::Sin(w * dt + p);
        }
        else if (s - (d * d) / 4.0f < 0.0) // Over Damped
        {
                float y0 = (d + FMath::Sqrt(d * d - 4 * s)) / 2.0f;
                float y1 = (d - FMath::Sqrt(d * d - 4 * s)) / 2.0f;
                float j1 = (c * y0 - x * y0 - v) / (y1 - y0);
                float j0 = x - j1 - c;

                float ey0dt = FMath::Exp(-y0 * dt);
                float ey1dt = FMath::Exp(-y1 * dt);

                x = j0 * ey0dt + j1 * ey1dt + c;
                v = -y0 * j0 * ey0dt - y1 * j1 * ey1dt;
        }
}

FVectorSpring::FVectorSpring(float HalfLife)
{
        this->halfLife = HalfLife;
        x = FVector::ZeroVector;
        v = FVector::ZeroVector;
        y = (6.64 * 0.69314718056f / this->halfLife) / 2.0;

}

void FVectorSpring::Update(FVector& g, const float dt)
{
        FVector j0 = x - g;
        FVector j1 = v + j0 * y;

        float eydt = FMath::Exp(-y * dt);

        x = eydt * (j0 + j1 * dt) + g;
        v = eydt * (v - j1 * y * dt);
        g = x;
}

FQuatSpring::FQuatSpring(float HalfLife)
{
        this->halfLife = HalfLife;
        q = FQuat::Identity;
        v = FVector::ZeroVector;
        y = (6.64 * 0.69314718056f / halfLife) / 2.0;
}

void FQuatSpring::Update(FQuat& g, const float dt)
{

        q = (q | g) > 0 ? q : q * -1.0f; // if the dot is negative it will try to rotate the longer way.
        FVector j0 = UFeatureComputation::QuatToScaledAngleAxis(q * g.Inverse());
        FVector j1 = v + j0 * y;

        float eydt = FMath::Exp(-y * dt);

        q = UFeatureComputation::QuatFromScaledAngleAxis(eydt * (j0 + j1 * dt)) * g;
        v = eydt * (v - j1 * y * dt);
        q.Normalize();
        g = q;
}

FTransformSpring::FTransformSpring(float HalfLife)
{
        this->halfLife = HalfLife;
        positionSpring = FVectorSpring(HalfLife);
        rotationSpring = FQuatSpring(HalfLife);
}

void FTransformSpring::Update(FTransform& g, const float dt, bool isDebug)
{
        FVector Pos = g.GetLocation();
        FQuat Rot = g.GetRotation();
        positionSpring.Update(Pos, dt);
        rotationSpring.Update(Rot, dt);

        g = FTransform(Rot, Pos);
}
