#version 450
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec4 inPos;

layout (location = 0) out int outInstanceIndex;



void main() {
	gl_Position = inPos;
	outInstanceIndex = gl_InstanceIndex;
}