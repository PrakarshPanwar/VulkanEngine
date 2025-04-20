// Module File: "Math.slang"

module Math;

public static const float PI = 3.14159265358979323846f;

public float3 Mix(float3 a, float3 b, float t)
{
    return a * (1.0f - t) + b * t;
}
