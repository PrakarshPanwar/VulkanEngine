// Shader File: "OITBlend.slang"

[shader("vertex")]
void VSMain(in uint id : SV_VertexID, out float4 position : SV_Position)
{
    float2 texCoord = float2((id << 1) & 2, id & 2);
    position = float4(texCoord * 2.0f - 1.0f, 0, 1);
} // End of Vertex Stage

[vk::binding(0, 0)] Sampler2D r_Accumulation;
[vk::binding(1, 0)] Sampler2D r_Revealage;

import Math;

[shader("fragment")]
void FSMain(float4 position : SV_Position, out float4 o_Color : SV_Target0)
{
    int2 texCoord = int2(position.xy);

    // Fragment Revealage
    float revealage = r_Revealage.Load(int3(texCoord, 0)).r;

    // Save the Blending and Color Texture fetching cost if there is not a Transparent Fragment
    if (IsEqual(revealage, 1.0f))
        discard;

    // Accumulated Color
    float4 accumulation = r_Accumulation.Load(int3(texCoord, 0));

    if (isinf(Max3(abs(accumulation.rgb)))) // Check for Invalid Color values
        accumulation.rgb = accumulation.a;

    // Prevent Floating Point Precision bug
    float3 average_color = accumulation.rgb / max(accumulation.a, EPSILON);

    o_Color = float4(average_color, 1.0f - revealage); // Final Color Output
}
