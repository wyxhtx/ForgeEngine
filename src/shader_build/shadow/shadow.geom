#version 450

#define lightCount 3

layout (triangles, invocations = lightCount) in;	//输入为三角形，		设置几何着色器调用次数
layout (triangle_strip, max_vertices = 3) out;		//输出为三角形条带，	最大顶点数为3

layout (binding = 0) uniform shadowUBO {
	mat4 mvp[lightCount];
	vec4 instancePos[lightCount];
} shadow;

layout (location = 0) in int inInstanceIndex[];



void main() {
	vec4 instancedPos = shadow.instancePos[inInstanceIndex[0]];			//获取当前图元第一个顶点的实例位置偏移

	for (int i = 0; i < gl_in.length(); i++) {
		gl_Layer = gl_InvocationID;										//设置输出到贴图数组的哪一层
		vec4 tmpPos = gl_in[i].gl_Position + instancedPos;				//将顶点位置加上实例偏移
		gl_Position = shadow.mvp[gl_InvocationID] * tmpPos;				//应用当前光源的MVP变换矩阵
		EmitVertex();													//发射顶点到输出流
	}

	EndPrimitive();														//结束图元装配
}