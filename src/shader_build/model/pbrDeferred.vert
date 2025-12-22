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



void main() {
    vec4 worldPosition = ubo.model * vec4(inPosition, 1.0);
    gl_Position  = ubo.proj * ubo.view * worldPosition;
    fragPosition = vec3(worldPosition);
    fragTexCoord = inTexCoord;
    fragNormal   = normalize(mat3(ubo.modelTI) * inNormal);
}