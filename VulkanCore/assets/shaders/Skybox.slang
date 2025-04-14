// Shader File : "Skybox.slang"

import Buffers;

static const float3 verticesData[36] = float3[](       
  float3(-1.0f,  1.0f, -1.0f),
  float3(-1.0f, -1.0f, -1.0f),
  float3( 1.0f, -1.0f, -1.0f),
  float3( 1.0f, -1.0f, -1.0f),
  float3( 1.0f,  1.0f, -1.0f),
  float3(-1.0f,  1.0f, -1.0f),
  
  float3(-1.0f, -1.0f,  1.0f),
  float3(-1.0f, -1.0f, -1.0f),
  float3(-1.0f,  1.0f, -1.0f),
  float3(-1.0f,  1.0f, -1.0f),
  float3(-1.0f,  1.0f,  1.0f),
  float3(-1.0f, -1.0f,  1.0f),
  
  float3( 1.0f, -1.0f, -1.0f),
  float3( 1.0f, -1.0f,  1.0f),
  float3( 1.0f,  1.0f,  1.0f),
  float3( 1.0f,  1.0f,  1.0f),
  float3( 1.0f,  1.0f, -1.0f),
  float3( 1.0f, -1.0f, -1.0f),
  
  float3(-1.0f, -1.0f,  1.0f),
  float3(-1.0f,  1.0f,  1.0f),
  float3( 1.0f,  1.0f,  1.0f),
  float3( 1.0f,  1.0f,  1.0f),
  float3( 1.0f, -1.0f,  1.0f),
  float3(-1.0f, -1.0f,  1.0f),
  
  float3(-1.0f,  1.0f, -1.0f),
  float3( 1.0f,  1.0f, -1.0f),
  float3( 1.0f,  1.0f,  1.0f),
  float3( 1.0f,  1.0f,  1.0f),
  float3(-1.0f,  1.0f,  1.0f),
  float3(-1.0f,  1.0f, -1.0f),
  
  float3(-1.0f, -1.0f, -1.0f),
  float3(-1.0f, -1.0f,  1.0f),
  float3( 1.0f, -1.0f, -1.0f),
  float3( 1.0f, -1.0f, -1.0f),
  float3(-1.0f, -1.0f,  1.0f),
  float3( 1.0f, -1.0f,  1.0f)
);

[[vk::binding(0, 0)]] ConstantBuffer<Camera> c_Camera;

[shader("vertex")]
void VSMain(in uint id : SV_VertexID, out float4 position : SV_Position, out float3 texCoord : TEXCOORD0)
{
	texCoord = verticesData[id];
	float3x3 viewRotation = float3x3(c_Camera.View);

	float4x4 viewMatrix = float4x4(
		float4(viewRotation[0], 0.0f),
		float4(viewRotation[1], 0.0f),
		float4(viewRotation[2], 0.0f),
		float4(0.0f, 0.0f, 0.0f, 1.0f)
	);

	float4 worldPosition = mul(mul(float4(texCoord, 1.0f), viewMatrix), c_Camera.Projection);
	position = worldPosition.xyww;
} // End of Vertex Stage

struct SkyboxData
{
	float Intensity;
	float LOD;
};

[[vk::push_constant]] ConstantBuffer<SkyboxData> c_Skybox;
[[vk::binding(1, 0)]] SamplerCube r_CubeMap;

[shader("fragment")]
void FSMain(in float3 texCoord : TEXCOORD0, out float4 o_Color : SV_Target)
{
	float3 color = r_CubeMap.SampleLevel(texCoord, c_Skybox.LOD).rgb;
	color *= c_Skybox.Intensity;

	o_Color = float4(color, 1.0f);
}
