#version 450

layout(binding = 1) uniform UniformBufferObject {
    vec3 viewPos;               //观察者坐标
    float currentHour;          //当前小时
    
    vec3 gLightColor;           //全局光照颜色
    float ambientScale;         //环境光缩放
    
    vec3 gLightPos;             //点光源坐标
    float diffuseScale;         //漫反射缩放
    
    vec3 gLightDir;             //平行光方向
    float specularScale;        //镜面反射缩放
    
    vec3 flashPos;              //聚光灯坐标
    float flashCutOff;          //聚光灯内圆锥

    vec3 flashDir;              //聚光灯方向
    float outerCutOff;          //聚光灯外圆锥
} light;

layout(binding = 2) uniform sampler2DArray texSampler;      //纹理贴图采样器

layout(location = 0) in vec3 fragPosition;      //坐标
layout(location = 1) in vec3 fragTexCoord;      //uv
layout(location = 2) in vec3 fragNormal;        //法线

layout(location = 0) out vec4 outColor;

//Blinn-Phong光照结构体
struct LightResult {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

const float constant  = 1.0f;       //衰减常数项
const float linear    = 0.045f;     //衰减一次项
const float quadratic = 0.0075f;    //衰减二次项

//todo 待删除
const float shininess         = 32.0f;  //光泽度
const float specularIntensity = 0.2f;



//点光源
LightResult getPointLight(vec3 normal, vec3 viewDir, vec3 texColor, float specularIntensity) {
    LightResult result;
    
    //计算从片段指向点光源的方向向量
    vec3 lightDir = normalize(light.gLightPos - fragPosition);
    
    //环境光
    result.ambient = vec3(0.0);
    
    //漫反射光照
    float diff = max(dot(normal, lightDir), 0.0);
    result.diffuse = texColor * light.diffuseScale * light.gLightColor * diff;
    
    //镜面反射光照
    vec3 halfwayDir = normalize(lightDir + viewDir);                //计算半程向量
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess); //使用法线与半程向量的点积
    result.specular = light.specularScale * light.gLightColor * specularIntensity * spec;
    
    return result;
}

//平行光
LightResult getDirectionalLight(vec3 normal, vec3 viewDir, vec3 texColor, float specularIntensity) {
    LightResult result;
    
    //平行光方向是固定的，不需要位置计算，因为方向是从光源指向表面，所以取反
    vec3 lightDir = normalize(-light.gLightDir);
    
    //环境光
    result.ambient = vec3(0.0);
    
    //漫反射光照
    float diff = max(dot(normal, lightDir), 0.0);
    result.diffuse = texColor * light.diffuseScale * light.gLightColor * diff;
    
    //镜面反射光照
    vec3 halfwayDir = normalize(lightDir + viewDir);                //计算半程向量
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess); //使用法线与半程向量的点积
    result.specular = light.specularScale * light.gLightColor * specularIntensity * spec;
    
    return result;
}

//聚光灯
LightResult getFlashLight(vec3 normal, vec3 viewDir, vec3 texColor, float specularIntensity) {
    LightResult result;
    
    //计算从片段指向聚光灯的方向向量
    vec3 lightDir = normalize(light.flashPos - fragPosition);
    
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
    
    //镜面反射光照
    vec3 halfwayDir = normalize(lightDir + viewDir);                //计算半程向量
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess); //使用法线与半程向量的点积
    result.specular = light.specularScale * light.gLightColor * specularIntensity * spec;
    
    //应用聚光灯强度插值
    result.diffuse *= intensity;
    result.specular *= intensity;
    
    return result;
}

//光源衰减
float getAttenuation(vec3 lightPos) {
    float distance = length(lightPos - fragPosition);
    return 1.0 / (constant + linear * distance + quadratic * (distance * distance));
}

void main() {
    vec4 texColor = texture(texSampler, fragTexCoord);              //纹理贴图读取rgb
    
    //透明度检测
    //if (texColor.a == 0.0) {
    //    discard;
    //}

    //归一化向量
    vec3 normal = normalize(fragNormal);
    vec3 viewDir = normalize(light.viewPos - fragPosition);

    //计算各种光源
    LightResult pointLight = getPointLight(normal, viewDir, texColor.rgb, specularIntensity);
    LightResult directionalLight = getDirectionalLight(normal, viewDir, texColor.rgb, specularIntensity);
    LightResult flashlight = getFlashLight(normal, viewDir, texColor.rgb, specularIntensity);
    
    //计算衰减（点光源和聚光灯需要衰减，平行光不需要）
    float pointAttenuation = getAttenuation(light.gLightPos);
    float flashAttenuation = getAttenuation(light.flashPos);
    
    //组合所有光照效果
    vec3 result = texColor.rgb * light.ambientScale * light.gLightColor;        //环境光
    result += (pointLight.diffuse + pointLight.specular) * pointAttenuation;    //点光源
    result += (directionalLight.diffuse + directionalLight.specular) * 0.2f;    //平行光
    result += (flashlight.diffuse + flashlight.specular) * flashAttenuation;    //聚光灯
    
    outColor = vec4(result, texColor.a);
}