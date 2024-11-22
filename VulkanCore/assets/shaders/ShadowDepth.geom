#version 460

layout(triangles, invocations = 4) in;
layout(triangle_strip, max_vertices = 3) out;

layout (set = 0, binding = 1) uniform CascadeData
{
    vec4 CascadeSplitLevels;
    mat4 LightSpaceMatrices[4];
} u_CascadeData;

void main()
{
    for (int i = 0; i < 3; ++i)
    {
        gl_Position = u_CascadeData.LightSpaceMatrices[gl_InvocationID] * gl_in[i].gl_Position;
        gl_Layer = gl_InvocationID;
        EmitVertex();
    }

    EndPrimitive();
}
