#version 460
#include "Utils/Structs.glslh"

layout(vertices = 3) out;

layout(location = 0) in TessellationOutput Input[];
layout(location = 0) out TessellationOutput Output[3];

#define TESSELLATION_LEVEL 1.0

void main()
{
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    Output[gl_InvocationID] = Input[gl_InvocationID];

    if (gl_InvocationID == 0)
    {
        gl_TessLevelInner[0] = TESSELLATION_LEVEL;
		gl_TessLevelOuter[0] = TESSELLATION_LEVEL;
		gl_TessLevelOuter[1] = TESSELLATION_LEVEL;
		gl_TessLevelOuter[2] = TESSELLATION_LEVEL;
    }
}
