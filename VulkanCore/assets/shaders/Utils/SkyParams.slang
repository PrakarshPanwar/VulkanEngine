module SkyParams;

// General Constants
public static const float SunIntensity = 3.0;
public static const float WorldScale = 500.0;
public static const float MaxDistance = 10000.0; // Distance of the aerial perspective LUT

// Atmosphere Parameters
public static const float PlanetRadius = 6378000.0;     // 6378 km
public static const float AtmosphereRadius = 6460000.0; // 6460 km
public static const float OzoneThickness = 30000.0;     // 30 km
public static const float OzoneCenterHeight = 25000.0;  // 25 km
public static const float HDensityMie = 1200.0;
public static const float AbsorbMie = 4.4 * 1e-6;
public static const float AsymmetryMie = 0.8;
public static const float ScatterMie = 3.996 * 1e-6;
public static const float HDensityRayleigh = 8000.0;
public static const float3 AbsorbOzone = float3(0.65, 1.881, 0.085) * 1e-6;
public static const float3 ScatterRayleigh = float3(5.802, 13.558, 33.1) * 1e-6;

// Computes SIGMA_T
public float3 GetSigmaT(float h)
{
    float3 rayleigh = ScatterRayleigh.xyz * exp(-h / HDensityRayleigh);
    float mie = (ScatterMie + AbsorbMie) * exp(-h / HDensityMie);
    float3 ozone = AbsorbOzone.xyz * max(0.0f, 1 - 0.5 * abs(h - OzoneCenterHeight) / OzoneThickness);

    return rayleigh + mie + ozone;
}

// RAYCASTING
public float2 IntersectRayAABB(float3 rayOrigin, float3 rayDir, float3 aabbMin, float3 aabbMax)
{
    float3 tMin = (aabbMin - rayOrigin) / rayDir;
    float3 tMax = (aabbMax - rayOrigin) / rayDir;
    float3 t1 = min(tMin, tMax);
    float3 t2 = max(tMin, tMax);
    float tEnter = max(t1.x, max(t1.y, t1.z));
    float tExit = min(t2.x, min(t2.y, t2.z));
    if (tExit < 0.0 || tEnter > tExit)
        return -1.0;

    return float2(tEnter, tExit);
}

public float2 IntersectRaySphere(float3 c, float r, float3 o, float3 d)
{
    float3 of = o - c;
    const float a = 1.0;
    float b = 2.0 * dot(of, d);
    float c = dot(of, of) - r * r;
    float discriminant = b * b - 4.0 * a * c;

    if (discriminant > 0)
    {
        float s = sqrt(discriminant);
        float dstToSphereNear = max(0.0, (-b - s) / (2.0 * a));
        float dstToSphereFar = (-b + s) / (2.0 * a);

        if (dstToSphereFar >= 0.0)
            return float2(dstToSphereNear, dstToSphereFar - dstToSphereNear);
    }

    return float2(0.0, 0.0);
}

public bool IntersectRayCircle(float2 o, float2 d, float R, out float t)
{
    float A = dot(d, d);
    float B = 2 * dot(o, d);
    float C = dot(o, o) - R * R;
    float delta = B * B - 4 * A * C;
    if (delta < 0)
        return false;

    t = (-B + (C <= 0 ? sqrt(delta) : -sqrt(delta))) / (2 * A);
    return (C <= 0) | (B <= 0);
}

public bool IntersectRaySphere(float3 o, float3 d, float R, out float t)
{
    float A = dot(d, d);
    float B = 2 * dot(o, d);
    float C = dot(o, o) - R * R;
    float delta = B * B - 4 * A * C;
    if (delta < 0)
        return false;

    t = (-B + (C <= 0 ? sqrt(delta) : -sqrt(delta))) / (2 * A);
    return (C <= 0) | (B <= 0);
}
