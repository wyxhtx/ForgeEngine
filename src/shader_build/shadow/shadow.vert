#version 450

layout (binding = 0) uniform shadowMvpUBO {
	mat4 mvp;
} shadowMvp;

layout (location = 0) in vec3 inPos;



void main() {
    gl_Position = shadowMvp.mvp * vec4(inPos, 1.0);
}