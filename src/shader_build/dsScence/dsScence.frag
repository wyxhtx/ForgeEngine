#version 450

layout(binding = 0) uniform UniformBufferObject {
    vec3 viewPos;               //观察者坐标
    float currentHour;          //当前小时
    
    vec3 gLightColor;           //全局光照颜色
    float ambientScale;         //环境光缩放
    
    vec3 gLightPos;             //点光源坐标
    float diffuseScale;         //漫反射缩放
    
    vec3 gLightDir;             //平行光方向
    float specularScale;        //镜面反射缩放
    
    vec3 flashPos;              //聚光灯坐标
    float flashCutOff;          //聚光灯内圆锥

    vec3 flashDir;              //聚光灯方向
    float outerCutOff;          //聚光灯外圆锥
} light;

layout (binding = 1) uniform sampler2D samplerPosition;
layout (binding = 2) uniform sampler2D samplerNormal;
layout (binding = 3) uniform sampler2D samplerAlbedo;

layout (location = 0) in vec2 fragUV;

layout (location = 0) out vec4 outFragcolor;



void main() {
  outFragcolor = texture(samplerAlbedo, fragUV);
}