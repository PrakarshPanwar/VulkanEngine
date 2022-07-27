#version 460 core
//#extension GL_NV_geometry_shader_passthrough : enable

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) out vec3 v_FragColor;
layout(location = 1) out vec3 v_FragPosWorld;
layout(location = 2) out vec3 v_FragNormalWorld;

layout(push_constant) uniform Push
{
	mat4 transform;
	mat4 normalMatrix;
	float timestep;
} push;

vec3 GetNormal()
{
   vec3 a = vec3(gl_in[0].gl_Position) - vec3(gl_in[1].gl_Position);
   vec3 b = vec3(gl_in[2].gl_Position) - vec3(gl_in[1].gl_Position);
   return normalize(cross(a, b));
}

vec4 Explode(vec4 position, vec3 normal)
{
    float magnitude = 2.0;
    vec3 direction = normal * (0.0) * magnitude; 
    return position + vec4(direction, 0.0);
}

void main()
{
	vec3 normal = GetNormal();

    gl_Position = Explode(gl_in[0].gl_Position, normal);
    v_FragColor = vec3(0.1, 0.3, 0.8);
    EmitVertex();
    gl_Position = Explode(gl_in[1].gl_Position, normal);
    v_FragColor = vec3(0.0, 1.0, 1.0);
    EmitVertex();
    gl_Position = Explode(gl_in[2].gl_Position, normal);
    v_FragColor = vec3(1.0, 1.0, 0.0);
    EmitVertex();
    EndPrimitive();

}