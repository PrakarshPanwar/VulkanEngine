#version 460 core
#extension GL_EXT_ray_tracing : require

struct RayPayload
{
	vec3 Color;
};

layout(location = 0) rayPayloadInEXT RayPayload o_RayPayload;

void main()
{
	o_RayPayload.Color = vec3(0.2, 0.3, 0.8);
}
