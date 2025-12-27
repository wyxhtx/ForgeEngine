#version 450

layout(binding = 0) uniform UniformBufferObject {
    vec3  viewPos;              //观察者坐标（世界空间）
    float currentHour;          //当前小时

    vec3  gLightColor;          //全局光照颜色
    float ambientScale;         //环境光缩放

    vec3  gLightPos;            //点光源坐标（世界空间）
    float diffuseScale;         //漫反射缩放

    vec3  gLightDir;            //平行光方向
    float specularScale;        //镜面反射缩放

    vec3  flashPos;             //聚光灯坐标（世界空间）
    float flashCutOff;          //聚光灯内圆锥

    vec3  flashDir;             //聚光灯方向
    float outerCutOff;          //聚光灯外圆锥
} light;

layout(binding = 1) uniform sampler2D samplerPosition;  //世界空间位置，兼前向渲染通道(a=0时)
layout(binding = 2) uniform sampler2D samplerNormal;    //世界空间法线
layout(binding = 3) uniform sampler2D samplerAlbedo;    //漫反射颜色(a：高光强度)

layout(location = 0) in  vec2 fragUV;
layout(location = 0) out vec4 outFragcolor;

struct LightResult {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

//衰减参数
const float constant  = 1.0;
const float linear    = 0.045;
const float quadratic = 0.0075;



//高光修正
float remapSpecular(float NoH, float roughness) {
    float shininess = exp2(7.0 + 8.0*(1.0 - roughness));    //4 ~ 1024
    float spec      = pow(max(NoH, 0.0), shininess);
    const float kThreshold = 0.98;                          //当 NoH 非常接近 1 时，人为压暗

    if (NoH > kThreshold) spec *= 1.0 - smoothstep(0.98, 0.999, NoH);
    return spec;
}

LightResult getPointLight(vec3 fragPos, vec3 normal, vec3 viewDir, vec3 texColor, float specularIntensity) {
    LightResult result;
    vec3 lightDir = normalize(light.gLightPos - fragPos);
    float NoL     = max(dot(normal, lightDir), 0.0);

    //漫反射：半兰伯特 + 上限
    float diffWrap = max(NoL, 0.1) * 0.9 + 0.1;
    result.diffuse = texColor * light.diffuseScale * light.gLightColor * diffWrap;

    //镜面反射：重映射
    vec3  halfwayDir = normalize(lightDir + viewDir);
    float NoH        = dot(normal, halfwayDir);
    float spec       = remapSpecular(NoH, 0.2);
    result.specular  = light.specularScale * light.gLightColor * specularIntensity * spec;

    result.ambient = vec3(0.0);
    return result;
}

LightResult getDirectionalLight(vec3 normal, vec3 viewDir, vec3 texColor, float specularIntensity) {
    LightResult result;
    vec3  lightDir = normalize(-light.gLightDir);
    float NoL      = max(dot(normal, lightDir), 0.0);

    //漫反射：半兰伯特 + 上限
    float diffWrap = max(NoL, 0.1) * 0.9 + 0.1;
    result.diffuse = texColor * light.diffuseScale * light.gLightColor * diffWrap;

    //镜面反射：重映射
    vec3  halfwayDir = normalize(lightDir + viewDir);
    float NoH        = dot(normal, halfwayDir);
    float spec       = remapSpecular(NoH, 0.2);
    result.specular  = light.specularScale * light.gLightColor * specularIntensity * spec;

    result.ambient = vec3(0.0);
    return result;
}

LightResult getFlashLight(vec3 fragPos, vec3 normal, vec3 viewDir, vec3 texColor, float specularIntensity) {
    LightResult result;
    vec3 lightDir = normalize(light.flashPos - fragPos);
    float NoL     = max(dot(normal, lightDir), 0.0);

    float theta     = dot(lightDir, normalize(-light.flashDir));
    float epsilon   = light.flashCutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    if (theta < light.outerCutOff) {
        float falloff = smoothstep(light.outerCutOff - 0.1, light.outerCutOff, theta);
        result.ambient = texColor * light.ambientScale * light.gLightColor * 0.05 * falloff;
        result.diffuse = vec3(0.0);
        result.specular= vec3(0.0);
        return result;
    }

    float ambientIntensity = 0.4 * intensity;                           //提亮暗面
    result.ambient = texColor * light.ambientScale * light.gLightColor * ambientIntensity;
    float diffWrap = max(NoL, 0.1) * 0.9 + 0.1;                         //最小10%
    result.diffuse = texColor * light.diffuseScale * light.gLightColor * diffWrap * intensity;

    vec3  halfwayDir = normalize(lightDir + viewDir);
    float NoH        = dot(normal, halfwayDir);
    float spec       = remapSpecular(NoH, 0.15);
    spec             = max(spec, 0.05);                                 //最小5%
    result.specular  = light.specularScale * light.gLightColor * specularIntensity * spec * intensity;

    return result;
}

float getAttenuation(vec3 fragPos, vec3 lightPos) {
    float dist = length(lightPos - fragPos);
    return 1.0 / (constant + linear * dist + quadratic * dist * dist);
}

void main() {
    vec4 positionData = texture(samplerPosition, fragUV);

    //直接呈现前向渲染内容
    if (positionData.a == 0.0) {
        outFragcolor = positionData;
        return;
    }

    vec4 normalData   = texture(samplerNormal,   fragUV);
    vec4 albedoData   = texture(samplerAlbedo,   fragUV);

    vec3 fragPos  = positionData.rgb;
    vec3 normal   = normalize(normalData.rgb);
    vec3 texColor = albedoData.rgb;
    float specularIntensity = albedoData.a;
    vec3 viewDir = normalize(light.viewPos - fragPos);

    LightResult pointLight       = getPointLight(fragPos, normal, viewDir, texColor, specularIntensity);
    LightResult directionalLight = getDirectionalLight(normal, viewDir, texColor, specularIntensity);
    LightResult flashlight       = getFlashLight(fragPos, normal, viewDir, texColor, specularIntensity);

    float pointAtt = getAttenuation(fragPos, light.gLightPos);
    float flashAtt = getAttenuation(fragPos, light.flashPos);

    vec3 result  = texColor * light.ambientScale * light.gLightColor;               //环境光
    result      += (pointLight.diffuse + pointLight.specular) * pointAtt;           //点光源
    result      += (directionalLight.diffuse + directionalLight.specular) * 0.2;    //平行光
    result      += (flashlight.diffuse  + flashlight.specular)  * flashAtt;         //聚光灯

    outFragcolor = vec4(result, 1.0);
}