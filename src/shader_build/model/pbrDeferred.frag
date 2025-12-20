#version 450

layout(binding = 1) uniform sampler2D samplerTexture;
layout(binding = 2) uniform sampler2D samplerNormal;

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outAlbedo;



void main() {
    outPosition = vec4(1.0, 0.0, 0.0, 1.0);
    outNormal   = vec4(0.0, 1.0, 0.0, 1.0);
    outAlbedo   = vec4(0.0, 0.0, 1.0, 1.0);
}