#version 450

//#define shadowModelCount 2

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
} ubo;

layout (location = 0) in vec3 inPos;



void main() {
    gl_Position = ubo.model * vec4(inPos, 1.0);
}