#version 450

layout(binding = 0) uniform mvpUBO {
    mat4 model;                                 //模型矩阵
    mat4 modelTI;                               //模型转置逆矩阵
    mat4 view;                                  //视图矩阵
    mat4 proj;                                  //投影矩阵
} mvp;

layout(location = 0) in vec3 inPosition;        //输入坐标
layout(location = 1) in vec3 inTexCoord;        //输入uv，贴图id
layout(location = 2) in vec3 inNormal;          //输入法线

layout(location = 0) out vec3 fragPosition;     //输出坐标
layout(location = 1) out vec3 fragTexCoord;     //输出uv
layout(location = 2) out vec3 fragNormal;       //输出法线



void main() {
    vec4 worldPosition = mvp.model * vec4(inPosition, 1.0);

    gl_Position  = mvp.proj * mvp.view * worldPosition;
    fragPosition = vec3(worldPosition);
	fragTexCoord = inTexCoord;
    fragNormal   = normalize(mat3(mvp.modelTI) * inNormal);
}