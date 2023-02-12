#version 460 core

layout(location = 0) out vec4 o_Color;
layout(location = 1) out vec4 o_ViewNormalsLuminance;

layout(location = 0) in vec3 v_FragColor;
layout(location = 1) in vec3 v_FragPosWorld;
layout(location = 2) in vec3 v_FragPosView;
layout(location = 3) in vec3 v_FragNormalWorld;
layout(location = 4) in vec2 v_FragTexCoord;
layout(location = 5) in flat int v_TexIndex;

struct PointLight
{
	vec4 Position;
	vec4 Color;
};

layout(set = 0, binding = 0) uniform Camera
{
	mat4 Projection;
	mat4 View;
	mat4 InvView;
} u_Camera;

layout(set = 0, binding = 1) uniform PointLightData
{
	vec4 AmbientLightColor;
	PointLight PointLights[10];
	int Count;
} u_PointLight;

layout(set = 0, binding = 2) uniform sampler2D u_DiffuseTex[3];
layout(set = 0, binding = 3) uniform sampler2D u_NormalTex[3];
layout(set = 0, binding = 4) uniform sampler2D u_SpecularTex[3];

void main()
{
	vec3 diffuseLight = u_PointLight.AmbientLightColor.xyz * u_PointLight.AmbientLightColor.w;
	vec3 specularLight = vec3(0.0);
	vec3 cameraPosWorld = u_Camera.InvView[3].xyz;
	vec3 viewDirection = normalize(cameraPosWorld - v_FragPosWorld);

	//vec3 surfaceNormal = normalize(v_FragNormalWorld);
	vec3 surfaceNormal = texture(u_NormalTex[v_TexIndex], v_FragTexCoord).rgb;
	surfaceNormal = normalize(surfaceNormal * 2.0 - 1.0);

	vec4 specColorMap = texture(u_SpecularTex[v_TexIndex], v_FragTexCoord);

	for (int i = 0; i < u_PointLight.Count; ++i)
	{
		PointLight pointLight = u_PointLight.PointLights[i];
		vec3 directionToLight = pointLight.Position.xyz - v_FragPosWorld;
		float attentuation = 1.0 / dot(directionToLight, directionToLight);
		directionToLight = normalize(directionToLight);
		float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0);
		vec3 intensity = pointLight.Color.xyz * pointLight.Color.w * attentuation;

		diffuseLight += intensity * cosAngIncidence;

		vec3 halfAngle = normalize(directionToLight + viewDirection);
		float blinnTerm = dot(surfaceNormal, halfAngle);
		blinnTerm = clamp(blinnTerm, 0, 1);
		blinnTerm = pow(blinnTerm, 16.0);
		specularLight += pointLight.Color.xyz * intensity * blinnTerm;
	}

	vec3 diffColorMap = texture(u_DiffuseTex[v_TexIndex], v_FragTexCoord).rgb;

	vec3 rDiffuse = diffuseLight * diffColorMap;
	vec3 rSpecular = specularLight * specColorMap.rgb;
	vec4 resColor = vec4((rDiffuse + rSpecular) * v_FragColor, 1.0);

	o_Color = resColor;
	o_ViewNormalsLuminance = vec4(v_FragPosView, 1.0);
}