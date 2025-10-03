#version 450

layout(binding = 1) uniform UniformBufferObject {
	vec3 baseLight;
	float ambientStrength;

	vec3 lightPos;
	float currentHour;
	
	vec3 viewPos;
	float specularStrength;
} light;

layout(binding = 2) uniform sampler2DArray texSampler;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragTexCoord;
layout(location = 2) in vec3 fragColor;
layout(location = 3) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 lightDir	= normalize(light.lightPos - fragPos);
	vec3 norm		= normalize(fragNormal);
	float diff		= max(dot(norm, lightDir), 0.0);
    vec3 diffuse	= diff * light.baseLight;

	vec3 viewDir	= normalize(light.viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
	float spec		= pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular	= light.specularStrength * spec * light.baseLight;

	//vec3 result = (light.ambientStrength + diffuse) * fragColor * texture(texSampler, fragTexCoord).rgb;
	vec3 result = (light.ambientStrength + diffuse + specular) * texture(texSampler, fragTexCoord).rgb;
    outColor = vec4(result, 1.0);
}