#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;

    float currentHour;
} ubo;

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out float fragHour;

void main() {
    gl_Position = ubo.proj * mat4(mat3(ubo.view)) * vec4(inPosition, 1.0);  //移除模型变换，只保留旋转
    gl_Position.z = gl_Position.w;                                          //强制天空盒永远绘制在场景的最远处
    
    fragPos = inPosition;
    fragHour = ubo.currentHour;
}