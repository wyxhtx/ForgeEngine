#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 modelTI;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;

out gl_PerVertex {
    vec4 gl_Position;
};



void main() {
    gl_Position  = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragPosition = vec3( ubo.model * vec4(inPosition, 1.0));
	fragTexCoord = inTexCoord;
    fragNormal   = mat3(ubo.modelTI) * inNormal;
}