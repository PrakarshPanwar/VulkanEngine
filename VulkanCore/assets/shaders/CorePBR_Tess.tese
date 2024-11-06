#version 460
#include "Utils/Structs.glslh"
#include "Utils/Buffers.glslh"
#include "Utils/Tessellation.glslh"

layout(triangles, equal_spacing, cw) in;

layout(set = 1, binding = 3) uniform sampler2D u_DisplacementMap;

layout(location = 0) in TessellationOutput Input[];
layout(location = 0) out TessellationOutput Output;

#define TESSELLATION_STRENGTH 0.01

void main()
{
	gl_Position = MixTess(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_in[2].gl_Position);

	Output.WorldNormals[0] = normalize(MixTess(Input[0].WorldNormals[0], Input[1].WorldNormals[0], Input[2].WorldNormals[0])); // Tangent
	Output.WorldNormals[1] = normalize(MixTess(Input[0].WorldNormals[1], Input[1].WorldNormals[1], Input[2].WorldNormals[1])); // Normal
	Output.WorldNormals[2] = normalize(MixTess(Input[0].WorldNormals[2], Input[1].WorldNormals[2], Input[2].WorldNormals[2])); // Bitangent
	
	vec2 texCoord = MixTess(Input[0].TexCoord, Input[1].TexCoord, Input[2].TexCoord); // Texture Coordinates
	Output.TexCoord = vec2(texCoord.x, 1.0 - texCoord.y);

	vec3 displacement = Output.WorldNormals[2] * (max(textureLod(u_DisplacementMap, Output.TexCoord, 0.0).r, 0.0) * TESSELLATION_STRENGTH);

	Output.WorldPosition = MixTess(Input[0].WorldPosition, Input[1].WorldPosition, Input[2].WorldPosition);
	Output.WorldPosition += displacement;
		
	gl_Position = u_Camera.Projection * u_Camera.View * vec4(Output.WorldPosition, 1.0);
}
