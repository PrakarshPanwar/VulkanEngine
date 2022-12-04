#version 460 core

layout(location = 0) out vec4 o_Color;

layout(location = 0) in vec3 v_FragColor;
layout(location = 1) in vec3 v_FragPosWorld;
layout(location = 2) in vec3 v_FragNormalWorld;
layout(location = 3) in vec2 v_FragTexCoord;
layout(location = 4) in flat int v_TexIndex;

struct PointLightData
{
	vec4 position;
	vec4 color;
};

layout(set = 0, binding = 0) uniform Camera
{
	mat4 projection;
	mat4 view;
	mat4 invView;
} u_Camera;

layout(set = 0, binding = 1) uniform PointLight
{
	vec4 ambientLightColor;
	PointLightData pointLights[10];
	int numLights;
} u_PointLight;

layout(set = 0, binding = 2) uniform sampler2D u_DiffuseTex[3];
layout(set = 0, binding = 3) uniform sampler2D u_NormalTex[3];
layout(set = 0, binding = 4) uniform sampler2D u_SpecularTex[3];

void main()
{
	vec3 diffuseLight = u_PointLight.ambientLightColor.xyz * u_PointLight.ambientLightColor.w;
	vec3 specularLight = vec3(0.0);
	vec3 cameraPosWorld = u_Camera.invView[3].xyz;
	vec3 viewDirection = normalize(cameraPosWorld - v_FragPosWorld);

	//vec3 surfaceNormal = normalize(v_FragNormalWorld);
	vec3 surfaceNormal = texture(u_NormalTex[v_TexIndex], v_FragTexCoord).rgb;
	surfaceNormal = normalize(surfaceNormal * 2.0 - 1.0);

	vec4 specColorMap = texture(u_SpecularTex[v_TexIndex], v_FragTexCoord);

	for (int i = 0; i < u_PointLight.numLights; i++)
	{
		PointLightData pointLight = u_PointLight.pointLights[i];
		vec3 directionToLight = pointLight.position.xyz - v_FragPosWorld;
		float attentuation = 1.0 / dot(directionToLight, directionToLight);
		directionToLight = normalize(directionToLight);
		float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0);
		vec3 intensity = pointLight.color.xyz * pointLight.color.w * attentuation;

		diffuseLight += intensity * cosAngIncidence;

		vec3 halfAngle = normalize(directionToLight + viewDirection);
		float blinnTerm = dot(surfaceNormal, halfAngle);
		blinnTerm = clamp(blinnTerm, 0, 1);
		blinnTerm = pow(blinnTerm, 16.0);
		specularLight += pointLight.color.xyz * intensity * blinnTerm;
	}

	vec3 diffColorMap = texture(u_DiffuseTex[v_TexIndex], v_FragTexCoord).rgb;

	vec3 rDiffuse = diffuseLight * diffColorMap;
	vec3 rSpecular = specularLight * specColorMap.rgb;
	vec4 resColor = vec4((rDiffuse + rSpecular) * v_FragColor, 1.0);

	o_Color = resColor;
}