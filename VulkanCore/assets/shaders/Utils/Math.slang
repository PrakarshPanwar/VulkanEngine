// Module File: "Math.slang"

module Math;

public static const float PI = 3.14159265358979323846f;
public static const float EPSILON = 1e-5f;

public float3 Mix(float3 a, float3 b, float t)
{
    return a * (1.0f - t) + b * t;
}

public float Max3(float3 v)
{
    return max(max(v.x, v.y), v.z);
}

public bool IsEqual(float a, float b)
{
    return abs(a - b) <= (abs(a) < abs(b) ? abs(b) : abs(a)) * EPSILON;
}
