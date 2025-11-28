#version 450

layout(location = 0) in vec3 fragPos;
layout(location = 1) in float fragHour;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

const vec3 dayTopColor    = vec3(0.15, 0.3, 0.6);       // 白天穹顶-天空蓝
const vec3 dayBottomColor = vec3(0.7, 0.8, 1.0);        // 白天穹底-白蓝
const vec3 nightSkyColor  = vec3(0.02, 0.03, 0.08);     // 夜空底色-深蓝黑
const vec3 starColor      = vec3(0.9, 0.9, 1.0);        // 星星颜色

// 简单的伪随机函数
float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

// 噪声函数用于生成更自然的星星分布
float noise(vec2 st) {
    vec2 i = floor(st);
    vec2 f = fract(st);
    
    float a = random(i);
    float b = random(i + vec2(1.0, 0.0));
    float c = random(i + vec2(0.0, 1.0));
    float d = random(i + vec2(1.0, 1.0));
    
    vec2 u = f * f * (3.0 - 2.0 * f);
    
    return mix(a, b, u.x) + 
           (c - a) * u.y * (1.0 - u.x) + 
           (d - b) * u.x * u.y;
}

// 生成星星
float generateStars(vec3 position, float time) {
    // 将球面坐标转换为用于星图生成的UV坐标
    vec3 normalizedPos = normalize(position);
    
    // 使用球面坐标生成稳定的星星图案
    float u = atan(normalizedPos.z, normalizedPos.x) / (2.0 * PI) + 0.5;
    float v = asin(normalizedPos.y) / PI + 0.5;
    
    // 缩放因子控制星星密度
    vec2 starUV = vec2(u, v) * 800.0;
    
    // 生成基础星星图案
    vec2 starPos = floor(starUV);
    float star = random(starPos);
    
    // 只有部分位置有星星
    if (star > 0.98) {
        // 星星的闪烁效果
        vec2 flickerUV = starUV * 2.0 + time * 0.1;
        float flicker = noise(flickerUV) * 0.3 + 0.7;
        
        // 星星的大小和亮度变化
        vec2 pixel = fract(starUV);
        float brightness = 1.0 - length(pixel - 0.5) * 2.0;
        brightness = pow(brightness, 8.0) * flicker;
        
        return brightness * star;
    }
    
    return 0.0;
}

void main() {
    // 归一化顶点坐标，得到单位球面上对应的坐标
    vec3 normalizedPos = normalize(fragPos);
    
    // 根据球面坐标，得到经纬度
    float longitude = atan(normalizedPos.z, normalizedPos.x) * 180.0 / PI;
    float latitude = asin(normalizedPos.y) * 180.0 / PI;
    


    // 根据时间决定是白天还是夜晚 (fragHour 范围 0-24)
    float dayNightMix = clamp((fragHour - 18.0) / 2.0, 0.0, 1.0) +  clamp((6.0 - fragHour) / 2.0, 0.0, 1.0);
    dayNightMix = clamp(dayNightMix, 0.0, 1.0);
    
    // 混合白天和夜空颜色
    vec3 daySky = mix(dayBottomColor, dayTopColor, (latitude + 90.0) / 180.0);
    vec3 nightSky = nightSkyColor;
    vec3 skyColor = mix(daySky, nightSky, dayNightMix);
    


    // 在夜晚生成星星
    float stars = 0.0;
    if (dayNightMix > 0.5) {
        // 使用时间参数让星星有轻微的闪烁
        stars = generateStars(fragPos, fragHour);
        // 星星在接近地平线时逐渐消失
        float horizonFade = 1.0 - pow(clamp(abs(latitude) / 90.0, 0.0, 1.0), 4.0);
        stars *= horizonFade * (dayNightMix - 0.5) * 2.0;
    }
    



    // 混合天空颜色和星星
    vec3 finalColor = mix(skyColor, starColor, stars);
    
    outColor = vec4(finalColor, 1.0);
}