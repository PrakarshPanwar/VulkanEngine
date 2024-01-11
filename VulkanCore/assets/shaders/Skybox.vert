#version 460 core

const vec3 verticesData[36] = vec3[](       
  vec3(-1.0,  1.0, -1.0),
  vec3(-1.0, -1.0, -1.0),
  vec3( 1.0, -1.0, -1.0),
  vec3( 1.0, -1.0, -1.0),
  vec3( 1.0,  1.0, -1.0),
  vec3(-1.0,  1.0, -1.0),
  
  vec3(-1.0, -1.0,  1.0),
  vec3(-1.0, -1.0, -1.0),
  vec3(-1.0,  1.0, -1.0),
  vec3(-1.0,  1.0, -1.0),
  vec3(-1.0,  1.0,  1.0),
  vec3(-1.0, -1.0,  1.0),
  
  vec3( 1.0, -1.0, -1.0),
  vec3( 1.0, -1.0,  1.0),
  vec3( 1.0,  1.0,  1.0),
  vec3( 1.0,  1.0,  1.0),
  vec3( 1.0,  1.0, -1.0),
  vec3( 1.0, -1.0, -1.0),
  
  vec3(-1.0, -1.0,  1.0),
  vec3(-1.0,  1.0,  1.0),
  vec3( 1.0,  1.0,  1.0),
  vec3( 1.0,  1.0,  1.0),
  vec3( 1.0, -1.0,  1.0),
  vec3(-1.0, -1.0,  1.0),
  
  vec3(-1.0,  1.0, -1.0),
  vec3( 1.0,  1.0, -1.0),
  vec3( 1.0,  1.0,  1.0),
  vec3( 1.0,  1.0,  1.0),
  vec3(-1.0,  1.0,  1.0),
  vec3(-1.0,  1.0, -1.0),
  
  vec3(-1.0, -1.0, -1.0),
  vec3(-1.0, -1.0,  1.0),
  vec3( 1.0, -1.0, -1.0),
  vec3( 1.0, -1.0, -1.0),
  vec3(-1.0, -1.0,  1.0),
  vec3( 1.0, -1.0,  1.0)
);

layout(set = 0, binding = 0) uniform Camera
{
	mat4 Projection;
	mat4 View;
	mat4 InverseView;
	vec2 DepthUnpackConsts;
} u_Camera;

layout(location = 0) out vec3 v_TexCoord;

void main()
{
	v_TexCoord = verticesData[gl_VertexIndex];
	mat4 viewMatrix = mat4(mat3(u_Camera.View));

	vec4 position = u_Camera.Projection * viewMatrix * vec4(v_TexCoord.xyz, 1.0);
	gl_Position = position.xyww;
}