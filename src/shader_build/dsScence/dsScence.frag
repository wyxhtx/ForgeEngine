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

    mat4 shadowMvp;
} light;

layout(binding = 1) uniform sampler2D samplerPosition;              //世界空间位置，rgb兼前向颜色，a兼前向标识
layout(binding = 2) uniform sampler2D samplerNormal;                //世界空间法线，a兼前向skybox标识
layout(binding = 3) uniform sampler2D samplerAlbedo;                //漫反射颜色，  a兼延迟高光强度
layout(binding = 4) uniform sampler2DArray samplerShadow_sss;       //阴影贴图

layout(location = 0) in  vec2 fragUV;

layout(location = 0) out vec4 outColor;



void main() {
    vec4 positionData = texture(samplerPosition, fragUV);
    if (positionData.a == 0.0) { outColor = positionData; return; }

    // 1. 读 G-Buffer
    vec3 worldPos = texture(samplerPosition, fragUV).rgb;
    vec3 albedo   = texture(samplerAlbedo,    fragUV).rgb;   // 漫反射颜色
    
    // 2. 投影到光源空间
    vec4 projPos   = light.shadowMvp * vec4(worldPos, 1.0);
    projPos.xyz   /= projPos.w;
    projPos.xyz    = projPos.xyz * 0.5 + 0.5;   // [-1,1] → [0,1]

    // 3. 视锥外强制无阴影
    if (projPos.z > 1.0 || projPos.z < 0.0 ||
        projPos.x > 1.0 || projPos.x < 0.0 ||
        projPos.y > 1.0 || projPos.y < 0.0)
    {
        outColor = vec4(albedo, 1.0);
        return;
    }

    // 4. 深度比较
    float currentDepth = projPos.z;
    float closestDepth = texture(samplerShadow_sss, vec3(projPos.xy, 0.0)).r;
    float bias         = 0.005;
    float shadow       = (currentDepth - bias) > closestDepth ? 1.0 : 0.0;

    // 5. 输出：有阴影=黑，无阴影=原漫反射颜色
    outColor = vec4(mix(albedo, vec3(0.0), shadow), 1.0);
}