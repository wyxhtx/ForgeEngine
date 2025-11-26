#version 450

layout(binding = 1) uniform UniformBufferObject {
    vec3 lightPos;                              //光照位置
    float currentHour;                          //当前小时

    vec3 viewPos;                               //观察者坐标
    float baseSpecular;                         //基础镜面反射
    
    vec3 baseLight;                             //基础光照颜色
    float baseAmbient;                          //基础环境光
} light;

layout(binding = 2) uniform sampler2DArray texSampler;      //纹理贴图采样器
layout(binding = 3) uniform sampler2DArray specularSampler; //高光贴图采样器

layout(location = 0) in vec3 fragPosition;      //坐标
layout(location = 1) in vec3 fragTexCoord;      //uv
layout(location = 2) in vec3 fragNormal;        //法线
layout(location = 3) in vec3 fragAmbient;       //材质环境光
layout(location = 4) in vec3 fragDiffuse;       //材质漫反射
layout(location = 5) in vec3 fragSpecular;      //材质镜面反射
layout(location = 6) in float fragShininess;    //材质反光度    

layout(location = 0) out vec4 outColor;

//光照系数
const vec3 ambientFactor  = vec3(0.2f, 0.2f, 0.2f);
const vec3 diffuseFactor  = vec3(0.5f, 0.5f, 0.5f); 
const vec3 specularFactor = vec3(1.0f, 1.0f, 1.0f);

void main() {
    vec4 texColor = texture(texSampler, fragTexCoord);
    
    // 高光贴图是R8G8格式，只包含红色和绿色通道
    vec2 specularData = texture(specularSampler, fragTexCoord).rg;  // 只取rg通道
    
    //使用纹理颜色作为材质属性
    vec3 materialAmbient = texColor.rgb * ambientFactor;
    vec3 materialDiffuse = texColor.rgb * diffuseFactor;
    
    //使用镜面反射贴图控制镜面反射强度
    // specularData.r 作为镜面反射强度，specularData.g 可以用于其他效果或忽略
    float specularIntensity = specularData.r;
    vec3 materialSpecular = vec3(specularIntensity) * specularFactor;

    //环境光照
    vec3 ambient = materialAmbient * light.baseLight * light.baseAmbient;

    //漫反射光照
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(light.lightPos - fragPosition);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * materialDiffuse * light.baseLight;

    //镜面反射光照
    vec3 viewDir = normalize(light.viewPos - fragPosition);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), fragShininess);
    vec3 specular = spec * materialSpecular * light.baseLight * light.baseSpecular;

    //组合所有光照分量
    vec3 result = ambient + diffuse + specular;
    outColor = vec4(result, texColor.a);
}