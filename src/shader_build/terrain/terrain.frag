#version 450

layout(binding = 1) uniform UniformBufferObject {
    vec3 viewPos;               //观察者坐标
    float currentHour;          //当前小时
    
    vec3 gLightColor;           //全局光照颜色
    float ambientScale;         //环境光缩放
    
    vec3 gLightPos;             //全局光照坐标
    float diffuseScale;         //漫反射缩放
    
    vec3 gLightDir;             //全局光照方向
    float specularScale;        //镜面反射缩放
} light;

layout(binding = 2) uniform sampler2DArray texSampler;      //纹理贴图采样器
layout(binding = 3) uniform sampler2DArray specularSampler; //高光贴图采样器

layout(location = 0) in vec3 fragPosition;      //坐标
layout(location = 1) in vec3 fragTexCoord;      //uv
layout(location = 2) in vec3 fragNormal;        //法线
layout(location = 3) in float fragShininess;    //材质光泽度

layout(location = 0) out vec4 outColor;

float constant  = 1.0f;     //衰减常数项
float linear    = 0.09f;    //衰减一次项
float quadratic = 0.032f;   //衰减二次项



void main() {
    vec4 texColor      = texture(texSampler, fragTexCoord);                     //纹理贴图读取rgb
    vec2 specularData  = texture(specularSampler, fragTexCoord).rg;             //高光贴图读取rg
    
    //透明度
    if (texColor.a == 0.0) {
        discard;
    }

    //环境光照
    vec3 ambient = texColor.rgb * light.ambientScale * light.gLightColor;
    
    //漫反射光照
    vec3 normal   = normalize(fragNormal);                                      //法线归一化
    vec3 lightDir = normalize(light.gLightPos - fragPosition);                  //计算从片段位置指向光源的方向向量，并归一化
    float diff    = max(dot(normal, lightDir), 0.0);                            //计算法线与光线方向的点积
    vec3 diffuse  = texColor.rgb * light.diffuseScale * light.gLightColor * diff;
    
    //镜面反射光照
    vec3 viewDir    = normalize(light.viewPos - fragPosition);                  //计算从片段位置指向观察者的方向向量，并归一化
    vec3 reflectDir = reflect(-lightDir, normal);                               //计算光线在表面上的反射方向
    float spec      = pow(max(dot(viewDir, reflectDir), 0.0), fragShininess);   //使用光泽度作为指数进行幂运算，控制高光集中程度
    vec3 specular   = light.specularScale * light.gLightColor * specularData.r * spec;
    
    //输出最终颜色
    vec3 result = ambient + diffuse + specular;
    outColor = vec4(result, texColor.a);
}