#version 450

layout(binding = 0) uniform shadowModelUBO {
    mat4 model;
} shadowModel;

layout (location = 0) in vec3 inPos;



void main() {
    gl_Position = shadowModel.model * vec4(inPos, 1.0);
}