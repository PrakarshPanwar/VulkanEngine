// Shader File : "ShadowDepth.slang"

struct VertexInput
{
    // Vertex Bindings
    [[vk::location(0)]] float3 Position;
    [[vk::location(1)]] float3 Normal;
    [[vk::location(2)]] float3 Tangent;
    [[vk::location(3)]] float3 Binormal;
    [[vk::location(4)]] float2 TexCoord;

    // Instance Bindings
    [[vk::location(5)]] float4 MRow0;
    [[vk::location(6)]] float4 MRow1;
    [[vk::location(7)]] float4 MRow2;
};

[shader("vertex")]
void VSMain(VertexInput vertexInput, out float4 position : SV_Position)
{
    float4x4 transform = float4x4(
		vertexInput.MRow0.x, vertexInput.MRow1.x, vertexInput.MRow2.x, 0.0f,
		vertexInput.MRow0.y, vertexInput.MRow1.y, vertexInput.MRow2.y, 0.0f,
        vertexInput.MRow0.z, vertexInput.MRow1.z, vertexInput.MRow2.z, 0.0f,
		vertexInput.MRow0.w, vertexInput.MRow1.w, vertexInput.MRow2.w, 1.0f
	);

    float4 worldPosition = mul(float4(vertexInput.Position, 1.0f), transform);
    position = worldPosition;
}

struct CascadeData
{
    float4 CascadeSplitLevels;
    float4x4 LightSpaceMatrices[4];
};

[[vk::binding(1, 0)]] ConstantBuffer<CascadeData> c_CascadeData;

struct GeometryOutput
{
    float4 Position : SV_Position;
    uint Layer      : SV_RenderTargetArrayIndex;
}

[shader("geometry")]
[maxvertexcount(3)]
void GSMain(triangle GeometryOutput input[3], uint GSInstanceID : SV_GSInstanceID, inout TriangleStream<GeometryOutput> outStream)
{
    for (int i = 0; i < 3; ++i)
    {
        GeometryOutput geomData = input[i];
        geomData.Position = mul(geomData.Position, c_CascadeData.LightSpaceMatrices[GSInstanceID]);
        geomData.Layer = GSInstanceID;

        outStream.Append(geomData);
    }

    outStream.RestartStrip();
}

[shader("fragment")]
void FSMain()
{
}
