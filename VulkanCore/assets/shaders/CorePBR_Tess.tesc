#version 460
#include "Utils/Structs.glslh"

layout(vertices = 3) out;

layout(location = 0) in TessellationOutput Input[];
layout(location = 0) out TessellationOutput Output[3];

void main()
{
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    Output[gl_InvocationID] = Input[gl_InvocationID];

    if (gl_InvocationID == 0)
    {
        const int MIN_TESS_LEVEL = 4;
        const int MAX_TESS_LEVEL = 64;
        const float MIN_DISTANCE = 20;
        const float MAX_DISTANCE = 800;

        vec4 viewSpacePosition00 = vec4(Input[0].ViewPosition, 1.0);
        vec4 viewSpacePosition01 = vec4(Input[1].ViewPosition, 1.0);
        vec4 viewSpacePosition10 = vec4(Input[2].ViewPosition, 1.0);
        vec4 viewSpacePosition11 = vec4(Input[3].ViewPosition, 1.0);

        // "Distance" from Camera scaled between 0 and 1
        float distance00 = clamp((abs(viewSpacePosition00.z) - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE), 0.0, 1.0);
        float distance01 = clamp((abs(viewSpacePosition01.z) - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE), 0.0, 1.0);
        float distance10 = clamp((abs(viewSpacePosition10.z) - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE), 0.0, 1.0);
        float distance11 = clamp((abs(viewSpacePosition11.z) - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE), 0.0, 1.0);

        float tessLevel0 = mix(MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance10, distance00));
        float tessLevel1 = mix(MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance00, distance01));
        float tessLevel2 = mix(MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance01, distance11));
        float tessLevel3 = mix(MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance11, distance10));

        gl_TessLevelOuter[0] = tessLevel0;
        gl_TessLevelOuter[1] = tessLevel1;
        gl_TessLevelOuter[2] = tessLevel2;
        gl_TessLevelOuter[3] = tessLevel3;

        gl_TessLevelInner[0] = max(tessLevel1, tessLevel3);
        gl_TessLevelInner[1] = max(tessLevel0, tessLevel2);
    }
}
