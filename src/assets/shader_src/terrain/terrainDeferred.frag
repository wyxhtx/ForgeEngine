#version 450

layout(binding = 1) uniform sampler2DArray samplerTexture;

layout(location = 0) in vec3 fragPosition;  //坐标
layout(location = 1) in vec3 fragTexCoord;  //uv
layout(location = 2) in vec3 fragNormal;    //法线

layout(location = 0) out vec4 outPosition;  //位置缓冲
layout(location = 1) out vec4 outNormal;    //法线缓冲
layout(location = 2) out vec4 outAlbedo;    //颜色



void main() {
    //透明度检测
    //if (texColor.a < 0.1) {
    //    discard;
    //}
    
    outPosition = vec4(fragPosition, 1.0);                  //位置
    outNormal   = vec4(fragNormal, 1.0);                    //法线
    outAlbedo   = texture(samplerTexture, fragTexCoord);    //颜色
}