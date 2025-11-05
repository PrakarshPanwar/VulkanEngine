// Shader File: "Text2D.slang"

import Buffers;

struct VertexInput
{
	// Vertex Bindings
    [vk::location(0)] float3 Position;
	[vk::location(1)] float4 Color;
    [vk::location(2)] float2 TexCoord;
};

[vk::binding(0, 0)] ConstantBuffer<Camera> c_Camera;

[shader("vertex")]
void VSMain(in VertexInput vertexInput, out float4 position : SV_Position, out float4 color : COLOR0, out float2 texCoord : TEXCOORD0)
{
	position = mul(mul(float4(vertexInput.Position, 1.0f), c_Camera.View), c_Camera.Projection);
	color = vertexInput.Color;
    texCoord = vertexInput.TexCoord;
}

[vk::binding(1, 0)] Sampler2D r_FontTexture;

[shader("fragment")]
void FSMain(float4 color : COLOR0, float2 texCoord : TEXCOORD0, out float4 o_Color : SV_Target)
{
	float4 finalColor = r_FontTexture.Sample(texCoord) * color;
	o_Color = float4(finalColor.rgb, finalColor.a);
}
