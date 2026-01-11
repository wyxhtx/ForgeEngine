#version 450

//光源数量
#define pointLightCount 1
#define dirLightCount   1
#define spotLightCount  1

//点光源结点
struct pointLightNode {
    vec3 position;              //位置
    float radius;               //半径

    vec3 color;                 //颜色
    float placeholder;          //占位符
};

//平行光结点
struct dirLightNode {
    vec3 direction;             //方向
    float scale;                //强度

    vec3 color;                 //颜色
    float placeholder;          //占位符
};

//聚光灯结点
struct spotLightNode {
    vec3 position;              //位置
    float inner;                //内圆锥

    vec3 color;                 //颜色
    float outer;                //外圆锥

    vec3 direction;             //方向
    float placeholder;          //占位符
};

layout(binding = 0) uniform UniformBufferObject {
    vec3  viewPos;              //观察者坐标（世界空间）
    float currentHour;          //当前小时

    float ambientScale;         //环境光强度
    float diffuseScale;         //漫反射强度
    float specularScale;        //镜面反射强度
    float placeholder;          //占位符

    pointLightNode pointLightList[pointLightCount];     //点光源
    dirLightNode   dirLightList[dirLightCount];         //平行光
    spotLightNode  spotLightList[spotLightCount];       //聚光灯
} light;

layout(binding = 1) uniform sampler2D samplerPosition;  //世界空间位置，rgb兼前向颜色，a兼前向标识
layout(binding = 2) uniform sampler2D samplerNormal;    //世界空间法线，a兼前向skybox标识
layout(binding = 3) uniform sampler2D samplerAlbedo;    //漫反射颜色，  a兼延迟高光强度
//layout(binding = 4) uniform sampler2DArray samplerShadow;    //阴影

layout(location = 0) in  vec2 fragUV;

layout(location = 0) out vec4 outColor;

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

//点光源计算函数
LightResult getPointLight(pointLightNode pointLight, vec3 fragPos, vec3 normal, vec3 viewDir, vec3 texColor, float specularIntensity) {
    LightResult result;
    vec3 lightDir = normalize(pointLight.position - fragPos);
    float NoL     = max(dot(normal, lightDir), 0.0);
    
    //计算距离和基于半径的衰减
    float distance = length(pointLight.position - fragPos);
    
    //纯半径衰减：在80%半径时开始平滑衰减
    float radiusAttenuation = 1.0 - smoothstep(pointLight.radius * 0.8, pointLight.radius, distance);
    
    //如果超出半径范围，直接返回空结果
    if (radiusAttenuation <= 0.0) {
        result.ambient = vec3(0.0);
        result.diffuse = vec3(0.0);
        result.specular = vec3(0.0);
        return result;
    }
    
    //只使用半径衰减，去掉物理衰减
    float totalAttenuation = radiusAttenuation;

    //漫反射：半兰伯特 + 上限
    float diffWrap = max(NoL, 0.1) * 0.9 + 0.1;
    result.diffuse = texColor * light.diffuseScale * pointLight.color * diffWrap * totalAttenuation;

    //镜面反射：重映射
    vec3  halfwayDir = normalize(lightDir + viewDir);
    float NoH        = dot(normal, halfwayDir);
    float spec       = remapSpecular(NoH, 0.2);
    result.specular  = light.specularScale * pointLight.color * specularIntensity * spec * totalAttenuation;

    result.ambient = vec3(0.0);
    return result;
}

//平行光计算函数
LightResult getDirectionalLight(dirLightNode dirLight, vec3 normal, vec3 viewDir, vec3 texColor, float specularIntensity) {
    LightResult result;
    vec3  lightDir = normalize(-dirLight.direction);
    float NoL      = max(dot(normal, lightDir), 0.0);

    //漫反射：半兰伯特 + 上限
    float diffWrap = max(NoL, 0.1) * 0.9 + 0.1;
    result.diffuse = texColor * light.diffuseScale * dirLight.color * diffWrap * dirLight.scale;

    //镜面反射：重映射
    vec3  halfwayDir = normalize(lightDir + viewDir);
    float NoH        = dot(normal, halfwayDir);
    float spec       = remapSpecular(NoH, 0.2);
    result.specular  = light.specularScale * dirLight.color * specularIntensity * spec * dirLight.scale;

    result.ambient = vec3(0.0);
    return result;
}

//聚光灯计算函数
LightResult getSpotLight(spotLightNode spotLight, vec3 fragPos, vec3 normal, vec3 viewDir, vec3 texColor, float specularIntensity) {
    LightResult result;
    vec3 lightDir = normalize(spotLight.position - fragPos);
    float NoL     = max(dot(normal, lightDir), 0.0);

    float theta     = dot(lightDir, normalize(-spotLight.direction));
    float epsilon   = spotLight.inner - spotLight.outer;
    float intensity = clamp((theta - spotLight.outer) / epsilon, 0.0, 1.0);

    if (theta < spotLight.outer) {
        float falloff = smoothstep(spotLight.outer - 0.1, spotLight.outer, theta);
        result.ambient = texColor * light.ambientScale * spotLight.color * 0.05 * falloff;
        result.diffuse = vec3(0.0);
        result.specular= vec3(0.0);
        return result;
    }

    float ambientIntensity = 0.4 * intensity;                           //提亮暗面
    result.ambient = texColor * light.ambientScale * spotLight.color * ambientIntensity;
    float diffWrap = max(NoL, 0.1) * 0.9 + 0.1;                         //最小10%
    result.diffuse = texColor * light.diffuseScale * spotLight.color * diffWrap * intensity;

    vec3  halfwayDir = normalize(lightDir + viewDir);
    float NoH        = dot(normal, halfwayDir);
    float spec       = remapSpecular(NoH, 0.15);
    spec             = max(spec, 0.05);                                 //最小5%
    result.specular  = light.specularScale * spotLight.color * specularIntensity * spec * intensity;

    return result;
}

//衰减计算函数
float getAttenuation(vec3 fragPos, vec3 lightPos) {
    float dist = length(lightPos - fragPos);
    return 1.0 / (constant + linear * dist + quadratic * dist * dist);
}

void main() {
    vec4 positionData = texture(samplerPosition, fragUV);
    if (positionData.a == 0.0) { outColor = positionData; return; }

    vec4 normalData = texture(samplerNormal, fragUV);
    vec4 albedoData = texture(samplerAlbedo, fragUV);

    vec3 fragPos = positionData.rgb;
    vec3 normal = normalize(normalData.rgb);
    vec3 texColor = albedoData.rgb;
    float specularIntensity = albedoData.a;
    vec3 viewDir = normalize(light.viewPos - fragPos);
    
    vec3 result = texColor * light.ambientScale * light.dirLightList[0].color;
    
    //skybox内容需要参与环境光light.ambientScale运算后呈现
    if (normalData.a == 0.0) {
        outColor = vec4(result, 1.0);
        return;
    }
    
    //点光源
    for (int i = 0; i < pointLightCount; i++) {
        LightResult pointLight = getPointLight(light.pointLightList[i], fragPos, normal, viewDir, texColor, specularIntensity);
        result += pointLight.diffuse + pointLight.specular;
    }
    
    //平行光
    for (int i = 0; i < dirLightCount; i++) {
        LightResult dirLight = getDirectionalLight(light.dirLightList[i], normal, viewDir, texColor, specularIntensity);
        result += dirLight.diffuse + dirLight.specular;
    }
    
    //聚光灯
    for (int i = 0; i < spotLightCount; i++) {
        LightResult spotLight = getSpotLight(light.spotLightList[i], fragPos, normal, viewDir, texColor, specularIntensity);
        float attenuation = getAttenuation(fragPos, light.spotLightList[i].position);
        result += spotLight.ambient + (spotLight.diffuse + spotLight.specular) * attenuation;
    }

    outColor = vec4(result, 1.0);
}