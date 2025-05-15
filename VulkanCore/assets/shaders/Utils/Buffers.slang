// Module File : "Buffers.slang"

module Buffers;

public struct Camera
{
    public float4x4 Projection;
    public float4x4 View;
    public float4x4 InverseView;
    public float2 DepthUnpackConsts;
};

public struct PointLight
{
    public float4 Position;
    public float4 Color;
    public float Radius;
    public float Falloff;
};

public struct SpotLight
{
    public float4 Position;
    public float4 Color;
    public float3 Direction;
    public float InnerCutoff; // Cutoff Threshold
    public float OuterCutoff; // Final Cutoff
    public float Radius;
    public float Falloff;
};

public struct DirectionalLight
{
    public float4 Color;
    public float3 Direction;
    public float Falloff;
};

public struct PointLightData
{
    public int Count;
    public PointLight PointLights[10];
};

public struct SpotLightData
{
    public int Count;
    public SpotLight SpotLights[10];
};

public struct DirectionalLightData
{
    public int Count;
    public DirectionalLight DirectionLights[2];
};
