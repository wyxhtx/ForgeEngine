#version 450

layout(binding = 1) uniform sampler2DArray texSampler;

layout(location = 0) in vec3 fragTexCoord;
layout(location = 2) in vec3 fragColor;
layout(location = 3) in vec3 fragNormal;
layout(location = 4) in vec3 fragBaseLight;
layout(location = 5) in float fragAmbientStrength;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(texSampler, fragTexCoord);
    //vec3 lightColor=ambientStrength * fragBaseLight;
    //outColor = vec4(fragColor * lightColor * texture(texSampler, vec2(cycledX, cycledY)).rgb, 1.0);
}
