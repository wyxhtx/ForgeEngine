#version 450

layout(binding = 1) uniform UniformBufferObject {
	vec3 baseLight;
	float ambientStrength;

	vec3 lightPos;
	float currentHour;

	vec3 viewPos;
	float specularStrength;
} light;

layout(location = 0) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

const vec3 nightColor     = vec3(0.05, 0.05, 0.15);     //深夜-深蓝
const vec3 dawnColor      = vec3(0.3, 0.2, 0.4);        //黎明-紫蓝
const vec3 dayTopColor    = vec3(0.15, 0.3, 0.6);       //白天穹顶-天空蓝
const vec3 dayBottomColor = vec3(0.7, 0.8, 1.0);        //白天穹底-白蓝
const vec3 duskColor      = vec3(0.8, 0.4, 0.2);        //黄昏-橙红
const vec3 eveColor       = vec3(0.15, 0.1, 0.25);      //傍晚-深紫
const vec3 horizonColor   = vec3(0.418, 0.394, 0.372);  //地平线-灰褐色

const float nightTime = 4;                              //深夜结束时间
const float dawnTime  = 6;                              //黎明结束时间
const float dayTime   = 16;                             //白天结束时间
const float duskTime  = 18;                             //黄昏结束时间
const float eveTime   = 20;                             //傍晚结束时间

const vec3 sunColor     = vec3(1.0, 0.8, 0.6);          //太阳颜色
const vec3 sunRimColor  = vec3(1.0, 0.66, 0.33);        //太阳光晕颜色
const float sunDiskSize = 8000.0;                       //控制太阳圆盘大小，值越大太阳越小
const float sunGlowSize = 4.0;                          //控制光晕大小，值越大光晕越小
const float sunAtmoSize = 8.0;                          //控制大气效果大小

const vec3 moonColor     = vec3(0.6, 0.6, 0.6);         //月亮颜色
const vec3 moonRimColor  = vec3(0.8, 0.8, 0.8);         //月亮光晕颜色
const float moonDiskSize = 10000.0;                     //控制月亮圆盘大小
const float moonGlowSize = 20.0;                        //控制光晕大小
const float moonAtmoSize = 16.0;                        //控制大气效果大小

const float sunNorthSouthOffset = 0.0;                  //太阳南北偏移系数，取值范围[-1,1]对应±45°，北正南负
const float moonNorthSouthOffset = 0.0;                 //月亮南北偏移系数，取值范围[-1,1]对应±45°，北正南负



//根据经纬度和时间，得到颜色
vec3 getSkyColor(float hour, float longitude, float latitude) {
    float blendFactor = (latitude + 90.0) / 180.0;      //将纬度从[-90,90]映射到[0,1]
    vec3 topColor;                                      //穹顶颜色
    vec3 bottomColor;                                   //穹底颜色

    if (hour < nightTime) {                         //深夜到黎明，[0,4)
        float time  = hour / nightTime;
        topColor    = mix(nightColor, dawnColor, time);
        bottomColor = nightColor;
    } 
    else if (hour < dawnTime) {                     //黎明到白天，[4,6)
        float time  = (hour - nightTime) / (dawnTime - nightTime);
        topColor    = mix(dawnColor, dayTopColor, time);
        bottomColor = mix(nightColor, dayBottomColor, time);
    } 
    else if (hour < dayTime) {                      //白天，[6,16)
        topColor    = dayTopColor;
        bottomColor = dayBottomColor;
    } 
    else if (hour < duskTime) {                     //白天到黄昏，[16,18)
        float time  = (hour - dayTime) / (duskTime - dayTime);
        topColor    = mix(dayTopColor, duskColor, time);
        bottomColor = mix(dayBottomColor, nightColor, time);
    }
    else if (hour < eveTime) {                      //黄昏到傍晚，[18,20)
        float time  = (hour - duskTime) / (eveTime - duskTime);
        topColor    = mix(duskColor, eveColor, time);
        bottomColor = nightColor;
    }
    else {                                              //傍晚到深夜，[20,24)
        float time  = (hour - eveTime) / (24 - eveTime);
        topColor    = mix(eveColor, nightColor, time);
        bottomColor = nightColor;
    }

    //线性插值，根据纬度混合颜色
    return mix(bottomColor, topColor, blendFactor);
}

//添加地平线效果
vec3 addHorizonEffect(vec3 skyColor, float y) {
    float horizonHeight = max(y, 0.0);
    
    //两层效果：基础层 + 亮边层
    float horizonBase = pow(1.0 - horizonHeight, 4.0);
    float horizonEdge = pow(1.0 - horizonHeight, 12.0);
    
    //基础层 - 强烈的暖色调
    vec3 horizonBaseColor = mix(skyColor, horizonColor * 1.5, 0.5);
    skyColor = mix(skyColor, horizonBaseColor, horizonBase * 0.8);
    
    //亮边层 - 明亮的边缘
    vec3 horizonEdgeColor = mix(skyColor, horizonColor * 2.0, 0.4);
    skyColor = mix(skyColor, horizonEdgeColor, horizonEdge * 0.6);
    
    //大气透视效果
    float atmosphericFade = pow(1.0 - horizonHeight, 2.0);
    
    return mix(skyColor, skyColor * 0.8, atmosphericFade * 0.5);
}

//根据时间计算太阳方向向量
vec3 getSunDirection(float hour) {
    float angle = (hour / 24.0) * 2.0 * PI;             //将24小时转换为弧度（0-2π）
    float sunriseAngle = 6.0 / 24.0 * 2.0 * PI;         //6点日出（东方），12点正午（最高点），18点日落（西方）
    float adjustedAngle = angle - sunriseAngle;
    float sunHeight = sin(adjustedAngle);               //太阳高度角：使用正弦函数模拟真实太阳轨迹
    float sunAzimuth = cos(adjustedAngle);              //太阳方位角：控制东西方向
    
    float x = sunAzimuth;
    float y = clamp(sunHeight, -0.3, 1.0);              //限制太阳最低高度
    float z = sin(adjustedAngle) * sunNorthSouthOffset; //控制南北偏移
    
    return normalize(vec3(x, y, z));
}

//计算太阳可见度
float getSunVisibility(float hour, vec3 sunDir) {
    if (sunDir.y < -0.1) return 0.0;                    //太阳在地平线以下时完全不可见
    
    return smoothstep(-0.1, 0.1, sunDir.y);
}

//添加太阳效果（光晕、圆盘、环境光）
vec3 addSunEffect(vec3 skyColor, vec3 fragDir, vec3 sunDir, float visibility) {
    vec3 finalColor = skyColor;
    
    if (visibility > 0.0) {
        //计算视线与太阳方向的夹角余弦
        float sunDot = dot(fragDir, sunDir);
        float clampedDot = clamp(sunDot, 0.0, 1.0);
        
        //太阳光晕效果（柔和的外光晕）
        float rimStrength = 0.4 * visibility;
        float rimFactor = pow(clampedDot, sunGlowSize);
        finalColor += rimStrength * sunRimColor * rimFactor;
        
        //太阳圆盘效果（锐利的核心）
        float diskStrength = 1.0 * visibility;
        float diskFactor = pow(clampedDot, sunDiskSize);
        finalColor += diskStrength * sunColor * diskFactor;
        
        //环境光照增强（太阳周围的天空更亮）
        float ambientStrength = 0.2 * visibility;
        float ambientFactor = pow(clamp(sunDot + 0.1, 0.0, 1.0), 2.0);
        finalColor += ambientStrength * sunColor * ambientFactor;
        
        //大气透射效果（太阳附近颜色混合）
        float atmosphericBlend = pow(clampedDot, sunAtmoSize) * 0.1;
        finalColor = mix(finalColor, sunColor, atmosphericBlend * visibility);
    }
    
    return finalColor;
}

//计算月亮方向向量
vec3 getMoonDirection(float hour) {
    float angle = (hour / 24.0) * 2.0 * PI;             //将24小时转换为弧度（0-2π）
    float moonriseAngle = 18.0 / 24.0 * 2.0 * PI;       //18点月出（东方），0点最高，6点月落（西方）
    float adjustedAngle = angle - moonriseAngle;
    float moonHeight = sin(adjustedAngle);              //月亮高度角：使用正弦函数模拟真实月亮轨迹
    float moonAzimuth = cos(adjustedAngle);             //月亮方位角：控制东西方向
    
    float x = moonAzimuth;
    float y = clamp(moonHeight, -0.3, 1.0);             //限制月亮最低高度
    float z = sin(adjustedAngle) * moonNorthSouthOffset;//控制南北偏移
    
    return normalize(vec3(x, y, z));
}

//计算月亮可见度
float getMoonVisibility(float hour, vec3 moonDir, float sunVisibility) {
    if (moonDir.y < -0.1) return 0.0;                   //月亮在地平线以下时完全不可见
    
    return smoothstep(-0.1, 0.1, moonDir.y) * (1.0 - sunVisibility);
}

//添加月亮效果（光晕、圆盘、环境光）
vec3 addMoonEffect(vec3 skyColor, vec3 fragDir, vec3 moonDir, float visibility) {
    vec3 finalColor = skyColor;
    
    if (visibility > 0.0) {
        //计算视线与月亮方向的夹角余弦
        float moonDot = dot(fragDir, moonDir); 
        float clampedDot = clamp(moonDot, 0.0, 1.0);
        
        //月亮光晕效果（柔和的外光晕）
        float rimStrength = 0.04 * visibility;
        float rimFactor = pow(clampedDot, moonGlowSize);
        finalColor += rimStrength * moonRimColor * rimFactor;
        
        //月亮圆盘效果（锐利的核心）
        float diskStrength = 1.0 * visibility;
        float diskFactor = pow(clampedDot, moonDiskSize);
        finalColor += diskStrength * moonColor * diskFactor;
        
        //环境光照增强（月亮周围的天空更亮）
        float ambientStrength = 0.2 * visibility;
        float ambientFactor = pow(clamp(moonDot + 0.1, 0.0, 1.0), 2.0);
        finalColor += ambientStrength * moonColor * ambientFactor;
        
        //大气透射效果（月亮附近颜色混合）
        float atmosphericBlend = pow(clampedDot, moonAtmoSize) * 0.1;
        finalColor = mix(finalColor, moonColor, atmosphericBlend * visibility);
    }
    
    return finalColor;
}

void main() {
    //归一化顶点坐标，得到单位球面上对应的坐标
    vec3 normalizedPos = normalize(fragPos);
    
    //根据球面坐标，得到经纬度
    float longitude = atan(normalizedPos.z, normalizedPos.x) * 180.0 / PI;
    float latitude  = asin(normalizedPos.y) * 180.0 / PI;
    
    vec3 skyColor       = getSkyColor(light.currentHour, longitude, latitude);                  //获取基础天空颜色
    skyColor            = addHorizonEffect(skyColor, normalizedPos.y);                          //添加地平线效果

    vec3 sunDir         = getSunDirection(light.currentHour);                                   //计算太阳方向
    float sunVisibility = getSunVisibility(light.currentHour, sunDir);                          //计算太阳可见度
    vec3 finalColor     = addSunEffect(skyColor, normalizedPos, sunDir, sunVisibility);         //添加太阳效果

    vec3 moonDir        = getMoonDirection(light.currentHour);                                  //计算月亮方向
    float moonVisibility= getMoonVisibility(light.currentHour, moonDir, sunVisibility);         //计算月亮可见度
    finalColor          = addMoonEffect(finalColor, normalizedPos, moonDir, moonVisibility);    //添加月亮效果
    
    outColor = vec4(finalColor, 1.0);
}