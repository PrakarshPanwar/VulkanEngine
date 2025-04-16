// Shader File : "Lines.slang"

import Buffers;

struct VertexInput
{
    [[vk::location(0)]] float3 Position;
    [[vk::location(1)]] float4 Color;
};

[[vk::binding(0, 0)]] ConstantBuffer<Camera> c_Camera;

[shader("vertex")]
void VSMain(VertexInput vertexInput, out float4 position : SV_Position, out float4 color : COLOR0)
{
    position = mul(mul(float4(vertexInput.Position, 1.0f), c_Camera.View), c_Camera.Projection);
} // End of Vertex Stage

[shader("fragment")]
void FSMain(in float4 color : COLOR0, out float4 o_Color : SV_Target)
{
    o_Color = float4(color.rgb, 1.0f);
} // End of Fragment Stage
