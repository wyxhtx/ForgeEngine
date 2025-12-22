#version 450

layout(binding = 0) uniform UniformBufferObject {
    vec3 viewPos;               //观察者坐标（世界空间）
    float currentHour;          //当前小时
    
    vec3 gLightColor;           //全局光照颜色
    float ambientScale;         //环境光缩放
    
    vec3 gLightPos;             //点光源坐标（世界空间）
    float diffuseScale;         //漫反射缩放
    
    vec3 gLightDir;             //平行光方向
    float specularScale;        //镜面反射缩放
    
    vec3 flashPos;              //聚光灯坐标（世界空间）
    float flashCutOff;          //聚光灯内圆锥
    
    vec3 flashDir;              //聚光灯方向
    float outerCutOff;          //聚光灯外圆锥
} light;

layout(binding = 1) uniform sampler2D samplerPosition;  //世界空间位置
layout(binding = 2) uniform sampler2D samplerNormal;    //世界空间法线
layout(binding = 3) uniform sampler2D samplerAlbedo;    //RGB: 漫反射颜色, A: 高光强度

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outFragcolor;

//Blinn-Phong光照结构体
struct LightResult {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

//光照衰减参数
const float constant  = 1.0f;     //衰减常数项
const float linear    = 0.045f;   //衰减一次项
const float quadratic = 0.0075f;  //衰减二次项

//点光源计算
LightResult getPointLight(vec3 fragPos, vec3 normal, vec3 viewDir, vec3 texColor, float specularIntensity) {
    LightResult result;
    
    //计算从片段指向点光源的方向向量
    vec3 lightDir = normalize(light.gLightPos - fragPos);
    
    //环境光
    result.ambient = vec3(0.0);
    
    //漫反射光照
    float diff     = max(dot(normal, lightDir), 0.0);
    result.diffuse = texColor * light.diffuseScale * light.gLightColor * diff;
    
    //镜面反射光照（Blinn-Phong模型）
    vec3 halfwayDir = normalize(lightDir + viewDir);                //计算半程向量
    float spec      = pow(max(dot(normal, halfwayDir), 0.0), 32.0); //使用固定光泽度
    result.specular = light.specularScale * light.gLightColor * specularIntensity * spec;
    
    return result;
}

//平行光计算
LightResult getDirectionalLight(vec3 normal, vec3 viewDir, vec3 texColor, float specularIntensity) {
    LightResult result;
    
    //平行光方向是固定的，不需要位置计算
    vec3 lightDir = normalize(-light.gLightDir);
    
    //环境光
    result.ambient = vec3(0.0);
    
    //反射光照
    float diff = max(dot(normal, lightDir), 0.0);
    result.diffuse = texColor * light.diffuseScale * light.gLightColor * diff;
    
    //镜面反射光照（Blinn-Phong模型）
    vec3 halfwayDir = normalize(lightDir + viewDir);                //计算半程向量
    float spec      = pow(max(dot(normal, halfwayDir), 0.0), 32.0); //使用固定光泽度
    result.specular = light.specularScale * light.gLightColor * specularIntensity * spec;
    
    return result;
}

//聚光灯计算
LightResult getFlashLight(vec3 fragPos, vec3 normal, vec3 viewDir, vec3 texColor, float specularIntensity) {
    LightResult result;
    
    //计算从片段指向聚光灯的方向向量
    vec3 lightDir = normalize(light.flashPos - fragPos);
    
    //检查是否在聚光灯圆锥内
    float theta = dot(lightDir, normalize(-light.flashDir));
    float epsilon = light.flashCutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    
    //如果不在聚光灯范围内，返回空结果
    if (theta < light.outerCutOff) {
        result.ambient = vec3(0.0);
        result.diffuse = vec3(0.0);
        result.specular = vec3(0.0);
        return result;
    }
    
    //环境光
    result.ambient = vec3(0.0);
    
    //漫反射光照
    float diff = max(dot(normal, lightDir), 0.0);
    result.diffuse = texColor * light.diffuseScale * light.gLightColor * diff;
    
    //镜面反射光照（Blinn-Phong模型）
    vec3 halfwayDir = normalize(lightDir + viewDir);                //计算半程向量
    float spec      = pow(max(dot(normal, halfwayDir), 0.0), 32.0); //使用固定光泽度
    result.specular = light.specularScale * light.gLightColor * specularIntensity * spec;
    
    //应用聚光灯强度插值
    result.diffuse *= intensity;
    result.specular *= intensity;
    
    return result;
}

//光源衰减计算
float getAttenuation(vec3 fragPos, vec3 lightPos) {
    float distance = length(lightPos - fragPos);
    return 1.0 / (constant + linear * distance + quadratic * (distance * distance));
}



void main() {
    //从G-Buffer读取数据
    vec4 positionData = texture(samplerPosition, fragUV);
    vec4 normalData   = texture(samplerNormal, fragUV);
    vec4 albedoData   = texture(samplerAlbedo, fragUV);
    
    //恢复数据
    vec3 fragPos  = positionData.rgb;
    vec3 normal   = normalize(normalData.rgb);
    vec3 texColor = albedoData.rgb;
    float specularIntensity = albedoData.a;
    
    //归一化向量
    vec3 viewDir = normalize(light.viewPos - fragPos);
    


    //计算各种光源
    LightResult pointLight       = getPointLight(fragPos, normal, viewDir, texColor, specularIntensity);
    LightResult directionalLight = getDirectionalLight(normal, viewDir, texColor, specularIntensity);
    LightResult flashlight       = getFlashLight(fragPos, normal, viewDir, texColor, specularIntensity);
    
    //计算衰减（点光源和聚光灯需要衰减，平行光不需要）
    float pointAttenuation = getAttenuation(fragPos, light.gLightPos);
    float flashAttenuation = getAttenuation(fragPos, light.flashPos);
    
    //组合所有光照效果
    vec3 result = texColor * light.ambientScale * light.gLightColor;            //环境光
    result += (pointLight.diffuse + pointLight.specular) * pointAttenuation;    //点光源
    result += (directionalLight.diffuse + directionalLight.specular);           //平行光
    result += (flashlight.diffuse + flashlight.specular) * flashAttenuation;    //聚光灯
    
    //输出最终颜色
    outFragcolor = vec4(result, 1.0);
}