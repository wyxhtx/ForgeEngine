#version 450

layout(binding = 1) uniform UniformBufferObject {
    vec3 viewPos;               //观察者坐标
    float currentHour;          //当前小时
    
    vec3 gLightColor;           //全局光照颜色
    float ambientScale;         //环境光缩放
    
    vec3 gLightPos;             //全局光照坐标
    float diffuseScale;         //漫反射缩放
    
    vec3 gLightDir;             //全局光照方向
    float specularScale;        //镜面反射缩放
} light;

layout(binding = 2) uniform sampler2D texSampler;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
	outColor = vec4(light.gLightColor * light.ambientScale * texture(texSampler, fragTexCoord).rgb, 1.0);
}