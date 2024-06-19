#version 460 core
#include "Utils/Buffers.glslh"

const vec4 Offsets[6] = vec4[](
  vec4(-1.0, -1.0, 0.0, 0.0),
  vec4( 1.0, -1.0, 1.0, 0.0),
  vec4( 1.0,  1.0, 1.0, 1.0),
  vec4( 1.0,  1.0, 1.0, 1.0),
  vec4(-1.0,  1.0, 0.0, 1.0),
  vec4(-1.0, -1.0, 0.0, 0.0)
);

layout(location = 0) out flat int v_InstanceID;

layout(push_constant) uniform LightSelectData
{
	vec4 Position;
	int EntityID;
} u_Light;

void main()
{
	vec4 offset = Offsets[gl_VertexIndex];
	vec3 cameraRightWorld = { u_Camera.View[0][0], u_Camera.View[1][0], u_Camera.View[2][0] };
	vec3 cameraUpWorld = { u_Camera.View[0][1], u_Camera.View[1][1], u_Camera.View[2][1] };

	vec3 positionWorld = u_Light.Position.xyz 
		+ u_Light.Position.w * offset.x * cameraRightWorld
		+ u_Light.Position.w * offset.y * cameraUpWorld;

	gl_Position = u_Camera.Projection * u_Camera.View * vec4(positionWorld, 1.0);
	v_InstanceID = u_Light.EntityID;
}