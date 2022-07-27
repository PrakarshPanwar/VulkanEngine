#version 460 core

layout(location = 0) out vec4 o_Color;

layout(location = 0) in vec3 v_FragColor;
layout(location = 1) in vec3 v_FragPosWorld;
layout(location = 2) in vec3 v_FragNormalWorld;
layout(location = 3) in vec2 v_FragTexCoord;
layout(location = 4) in flat int v_TexIndex;

struct PointLight
{
	vec4 position;
	vec4 color;
};

layout(set = 0, binding = 0) uniform GlobalUniformBuffer
{
	mat4 projection;
	mat4 view;
	mat4 invView;
	vec4 ambientLightColor;
	PointLight pointLights[10];
	int numLights;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D texDiffSamplers[3];
layout(set = 0, binding = 2) uniform sampler2D texNormSamplers[3];
layout(set = 0, binding = 3) uniform sampler2D texSpecSamplers[3];

void main()
{
	vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
	vec3 specularLight = vec3(0.05);
	vec3 cameraPosWorld = ubo.invView[3].xyz;
	vec3 viewDirection = normalize(cameraPosWorld - v_FragPosWorld);

	//vec3 surfaceNormal = normalize(v_FragNormalWorld);
	vec3 surfaceNormal = texture(texNormSamplers[v_TexIndex], v_FragTexCoord).rgb;
	surfaceNormal = normalize(surfaceNormal * 2.0 - 1.0);

	vec4 specColorMap = texture(texSpecSamplers[v_TexIndex], v_FragTexCoord);

	for (int i = 0; i < ubo.numLights; i++)
	{
		PointLight pointLight = ubo.pointLights[i];
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

	vec3 diffColorMap = texture(texDiffSamplers[v_TexIndex], v_FragTexCoord).rgb;

	vec3 rDiffuse = diffuseLight * diffColorMap;
	vec3 rSpecular = specularLight * specColorMap.rgb;
	vec4 resColor = vec4((rDiffuse + rSpecular) * v_FragColor, 1.0);
	o_Color = resColor;
}