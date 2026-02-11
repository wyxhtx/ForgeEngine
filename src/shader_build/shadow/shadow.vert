#version 450

layout (binding = 0) uniform shadowMvpUBO {
    mat4 mvp;
} shadowMvp;

layout (location = 0) in vec3 inPos;

//鱼眼变换函数，放大中心压缩边缘
vec2 getFishEyeCoord(vec2 positionInNdcCoord) {
    return positionInNdcCoord / (0.5 + 0.5 * length(positionInNdcCoord.xy));
}



void main() {
    vec4 pos = shadowMvp.mvp * vec4(inPos, 1.0);
    pos.xy = getFishEyeCoord(pos.xy);    //在透视除法前应用鱼眼变换到裁剪坐标，这里操作的是裁剪空间坐标，w分量保留用于后续硬件透视除法
    gl_Position = pos;
}