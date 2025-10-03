#version 450

layout(binding = 1) uniform UniformBufferObject {
	vec3 baseLight;
	float ambientStrength;

	vec3 lightPos;
	float currentHour;

	vec3 viewPos;
	float specularStrength;
} light;

layout(binding = 2) uniform sampler2D texSampler;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
	outColor = vec4(light.baseLight * light.ambientStrength * texture(texSampler, fragTexCoord).rgb, 1.0);
}