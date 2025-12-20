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
    // 位置信息（包含世界空间坐标）
    outPosition = vec4(fragPosition, 1.0);
    
    // 法线信息（标准化到[-1,1] -> [0,1]）
    outNormal = vec4(normalize(fragNormal) * 0.5 + 0.5, 1.0);
    
    // 纹理颜色 + 镜面反射强度
    vec4 textureColor = texture(samplerTexture, fragTexCoord);
    float specularIntensity = 0.5; // 你可以从纹理中获取或设置为固定值
    
    outAlbedo = vec4(textureColor.rgb, specularIntensity);
}