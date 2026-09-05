#version 450

layout (location = 0) in vec2 inUV;

layout (binding = 0) uniform sampler2D samplerFont;

layout (location = 0) out vec4 outColor;

//MSDF 三通道距离取中值（MTSDF：RGB=距离场，A=中轴不参与绘制）
float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

void main(void) {
    vec3  msd = texture(samplerFont, inUV).rgb;
    float sdf = median(msd.r, msd.g, msd.b);

    //fwidth 自适应抗锯齿：0.5 等值面为字形边缘
    float aa    = fwidth(sdf);
    float alpha = smoothstep(0.5 - aa, 0.5 + aa, sdf);

    if (alpha < 0.02) {
        discard;
    }

    outColor = vec4(1.0, 1.0, 1.0, alpha);
}