#version 450

#define shadowMapSize 3

layout (triangles, invocations = shadowMapSize) in;	//输入为三角形，		设置几何着色器调用次数
layout (triangle_strip, max_vertices = 3) out;		//输出为三角形条带，	最大顶点数为3

layout (binding = 0) uniform shadowUBO {
	mat4 mvp[shadowMapSize];
} shadow;



void main() {
	for (int i = 0; i < gl_in.length(); i++) {
		gl_Layer = gl_InvocationID;											//设置输出到贴图数组的哪一层
		gl_Position = shadow.mvp[gl_InvocationID] * gl_in[i].gl_Position;	//应用当前光源的MVP变换矩阵

		EmitVertex();														//发射顶点到输出流
	}

	EndPrimitive();															//结束图元装配
}