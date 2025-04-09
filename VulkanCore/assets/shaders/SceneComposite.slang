// Shader File: "SceneComposite.slang"

[shader("vertex")]
void VSMain(in uint id : SV_VertexID, out float4 position : SV_Position, out float2 texCoord : TEXCOORD0)
{
    position = float4(0, 0, 0, 1);
    texCoord = float2((id << 1) & 2, id & 2);
}

[shader("fragment")]
void FSMain(in float2 texCoord : TEXCOORD0, out float4 color : SV_Target)
{
    color = float4(texCoord.x, texCoord.y, 0.0, 1.0);
}
