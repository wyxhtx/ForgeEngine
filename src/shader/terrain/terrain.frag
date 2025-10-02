#version 450

layout(binding = 1) uniform UniformBufferObject {
	vec3 baseLight;
	float ambientStrength;

	vec3 lightPos;
	float currentHour;
} light;

layout(binding = 2) uniform sampler2DArray texSampler;

layout(location = 0) in vec3 fragTexCoord;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor * light.baseLight * light.ambientStrength * texture(texSampler, fragTexCoord).rgb, 1.0);
}


