#version 460 core

const vec4 Offsets[6] = vec4[](
  vec4(-1.0, -1.0, 0.0, 0.0),
  vec4( 1.0, -1.0, 1.0, 0.0),
  vec4( 1.0,  1.0, 1.0, 1.0),
  vec4( 1.0,  1.0, 1.0, 1.0),
  vec4(-1.0,  1.0, 0.0, 1.0),
  vec4(-1.0, -1.0, 0.0, 0.0)
);

layout(location = 0) out vec2 v_FragOffset;
layout(location = 1) out vec2 v_TexCoord;

layout(binding = 0) uniform Camera
{
	mat4 Projection;
	mat4 View;
    mat4 InverseProjection;
	mat4 InverseView;
} u_Camera;

layout(push_constant) uniform PointLight
{
	vec4 Position;
} u_Light;

void main()
{
	vec4 offset = Offsets[gl_VertexIndex];
	v_FragOffset = offset.xy;
	v_TexCoord = vec2(offset.z, 1.0 - offset.w);

	vec3 cameraRightWorld = { u_Camera.View[0][0], u_Camera.View[1][0], u_Camera.View[2][0] };
	vec3 cameraUpWorld = { u_Camera.View[0][1], u_Camera.View[1][1], u_Camera.View[2][1] };

	vec3 positionWorld = u_Light.Position.xyz 
		+ u_Light.Position.w * v_FragOffset.x * cameraRightWorld
		+ u_Light.Position.w * v_FragOffset.y * cameraUpWorld;

	gl_Position = u_Camera.Projection * u_Camera.View * vec4(positionWorld, 1.0);
}