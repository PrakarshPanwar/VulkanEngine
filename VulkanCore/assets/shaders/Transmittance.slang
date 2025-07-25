import SkyParams;

// This value can be lowered for in production to around 40 without any visible difference
#define STEP_COUNT 128

[vk::binding(0, 0)] RWTexture2D<float4> o_Transmittance;

// Performs basic raymarching to evaluate the transmittance
[shader("compute")]
[numthreads(16, 16, 1)]
void computeMain(int3 threadID : SV_DispatchThreadID)
{
    int width, height;
    o_Transmittance.GetDimensions(width, height);
    if (threadID.x >= width || threadID.y >= height)
        return;

    // Indexed by Height and Sun theta
    float theta = asin(lerp(-1.0, 1.0, float(threadID.y + .5) / float(height)));
    float h = lerp(0.0, AtmosphereRadius - PlanetRadius, float(threadID.x + .5) / float(width));

    float2 o = float2(0, PlanetRadius + h);
    float2 d = float2(cos(theta), sin(theta));

    // Find the Intersection with the Planet or Atmosphere
    float t = 0;
    if (!IntersectRayCircle(o, d, PlanetRadius, t))
        IntersectRayCircle(o, d, AtmosphereRadius, t);
        
    // Compute the Transmittance for all theta and h
    float2 end = o + t * d;
    float3 sum = 0.;
    for(int i = 0; i < STEP_COUNT; ++i)
    {
        float2 pi = lerp(o, end, float(i) / float(STEP_COUNT));
        float hi = length(pi) - PlanetRadius;
        float3 sigma = GetSigmaT(hi);
        sum += sigma;
    }

    float3 result = exp(-sum * (t / float(STEP_COUNT)));
    o_Transmittance[threadID.xy] = float4(result, 1);
}
