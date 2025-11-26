#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;                                 //模型矩阵
    mat4 modelTI;                               //模型转置逆矩阵
    mat4 view;                                  //视图矩阵
    mat4 proj;                                  //投影矩阵
} ubo;

layout(location = 0) in vec3 inPosition;        //输入坐标
layout(location = 1) in vec3 inTexCoord;        //输入uv
layout(location = 2) in vec3 inNormal;          //输入法线
layout(location = 3) in vec3 inAmbient;         //输入材质环境光
layout(location = 4) in vec3 inDiffuse;         //输入材质漫反射
layout(location = 5) in vec3 inSpecular;        //输入材质镜面反射
layout(location = 6) in float inShininess;      //输入材质反光度

layout(location = 0) out vec3 fragPosition;     //输出坐标
layout(location = 1) out vec3 fragTexCoord;     //输出uv
layout(location = 2) out vec3 fragNormal;       //输出法线
layout(location = 3) out vec3 fragAmbient;      //输出材质环境光
layout(location = 4) out vec3 fragDiffuse;      //输出材质漫反射
layout(location = 5) out vec3 fragSpecular;     //输出材质镜面反射
layout(location = 6) out float fragShininess;   //输出材质反光度



void main() {
    gl_Position   = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragPosition  = vec3( ubo.model * vec4(inPosition, 1.0));
	fragTexCoord  = inTexCoord;
    fragNormal    = mat3(ubo.modelTI) * inNormal;
    fragAmbient   = inAmbient;
    fragDiffuse   = inDiffuse;
    fragSpecular  = inSpecular;
    fragShininess = inShininess;
}