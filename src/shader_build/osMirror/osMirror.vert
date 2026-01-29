#version 450

layout(binding = 0) uniform mvpUBO {
    mat4 model;
    mat4 modelTI;
    mat4 view;
    mat4 proj;
} mvp;

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec4 fragPos;



void main() {
    fragPos = mvp.proj * mvp.view * mvp.model * vec4(inPosition, 1.0);
	gl_Position = fragPos;
}