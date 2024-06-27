#version 460 core
#extension GL_EXT_ray_tracing : require

#include "Utils/Payload.glslh"

layout(location = 0) rayPayloadInEXT RayPayload o_RayPayload;
layout(location = 1) rayPayloadInEXT bool o_CastShadow;

void main()
{
	o_CastShadow = false;
}
