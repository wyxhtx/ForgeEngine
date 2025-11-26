#version 450

layout(binding = 1) uniform UniformBufferObject {
	vec3 lightPos;                              //光照位置
	float currentHour;                          //当前小时

	vec3 viewPos;                               //观察者坐标
	float baseSpecular;                         //基础镜面反射
	
	vec3 baseLight;                             //基础光照颜色
	float baseAmbient;                          //基础环境光
} light;

layout(binding = 2) uniform sampler2D texSampler;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
	outColor = vec4(light.baseLight * light.baseAmbient * texture(texSampler, fragTexCoord).rgb, 1.0);
}