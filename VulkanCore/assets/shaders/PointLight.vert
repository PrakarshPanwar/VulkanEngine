#version 460 core

const vec2 Offsets[6] = vec2[](
  vec2(-1.0, -1.0),
  vec2(-1.0,  1.0),
  vec2( 1.0, -1.0),
  vec2( 1.0, -1.0),
  vec2(-1.0,  1.0),
  vec2( 1.0,  1.0)
);

layout(location = 0) out vec2 v_FragOffset;

layout(set = 0, binding = 0) uniform Camera
{
	mat4 Projection;
	mat4 View;
	mat4 InverseView;
	vec2 DepthUnpackConsts;
	vec2 CameraTanHalfFOV;
} u_Camera;

layout(push_constant) uniform PointLight
{
	vec4 Position;
	vec4 Color;
	float Radius;
} u_PointLight;

void main()
{
	v_FragOffset = Offsets[gl_VertexIndex];
	vec3 cameraRightWorld = { u_Camera.View[0][0], u_Camera.View[1][0], u_Camera.View[2][0] };
	vec3 cameraUpWorld = { u_Camera.View[0][1], u_Camera.View[1][1], u_Camera.View[2][1] };

	vec3 positionWorld = u_PointLight.Position.xyz 
		+ u_PointLight.Radius * v_FragOffset.x * cameraRightWorld
		+ u_PointLight.Radius * v_FragOffset.y * cameraUpWorld;

	gl_Position = u_Camera.Projection * u_Camera.View * vec4(positionWorld, 1.0);
}