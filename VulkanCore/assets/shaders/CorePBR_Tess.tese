#version 460
#include "Utils/Structs.glslh"
#include "Utils/Buffers.glslh"
#include "Utils/Tessellation.glslh"

layout(triangles, equal_spacing, cw) in;

layout(set = 1, binding = 3) uniform sampler2D u_DisplacementMap;

layout(location = 0) in TessellationOutput Input[];
layout(location = 0) out TessellationOutput Output;

#define TESSELLATION_STRENGTH 0.5

void main()
{
	vec3 tangent0 = mat3(Input[0].Transform) * Input[0].WorldNormals[0];
	vec3 tangent1 = mat3(Input[1].Transform) * Input[1].WorldNormals[0];
	vec3 tangent2 = mat3(Input[2].Transform) * Input[2].WorldNormals[0];

	vec3 normal0 = mat3(Input[0].Transform) * Input[0].WorldNormals[1];
	vec3 normal1 = mat3(Input[1].Transform) * Input[1].WorldNormals[1];
	vec3 normal2 = mat3(Input[2].Transform) * Input[2].WorldNormals[1];

	vec3 binormal0 = mat3(Input[0].Transform) * Input[0].WorldNormals[2];
	vec3 binormal1 = mat3(Input[1].Transform) * Input[1].WorldNormals[2];
	vec3 binormal2 = mat3(Input[2].Transform) * Input[2].WorldNormals[2];

	Output.WorldNormals[0] = normalize(MixTess(tangent0, tangent1, tangent2));	  // Tangent
	Output.WorldNormals[1] = normalize(MixTess(normal0, normal1, normal2));		  // Normal
	Output.WorldNormals[2] = normalize(MixTess(binormal0, binormal1, binormal2)); // Bitangent

	vec3 displacement = normalize(Output.WorldNormals[1]) * (max(textureLod(u_DisplacementMap, Output.TexCoord, 0.0).rgb, 0.0) * TESSELLATION_STRENGTH);

	vec3 worldPosition0 = vec3(Input[0].Transform * vec4(Input[0].WorldPosition, 1.0));
	vec3 worldPosition1 = vec3(Input[1].Transform * vec4(Input[1].WorldPosition, 1.0));
	vec3 worldPosition2 = vec3(Input[2].Transform * vec4(Input[2].WorldPosition, 1.0));

	Output.WorldPosition = MixTess(worldPosition0, worldPosition1, worldPosition2) + displacement;
	Output.TexCoord = MixTess(Input[0].TexCoord, Input[1].TexCoord, Input[2].TexCoord);
		
	gl_Position = u_Camera.Projection * u_Camera.View * vec4(Output.WorldPosition, 1.0);
}
