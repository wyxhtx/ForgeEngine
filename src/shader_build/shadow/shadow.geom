#version 450

layout (triangles, invocations = 1) in;					//输入为三角形，		设置几何着色器调用次数
layout (triangle_strip, max_vertices = 3) out;			//输出为三角形条带，	最大顶点数为3

layout (binding = 1) uniform shadowMvpUBO {
	mat4 mvp;
} shadowMvp;



void main() {
	for (int i = 0; i < gl_in.length(); i++) {				//设置输出到贴图数组的哪一层
		gl_Position = shadowMvp.mvp * gl_in[i].gl_Position;	//应用当前光源的MVP变换矩阵

		EmitVertex();										//发射顶点到输出流
	}

	EndPrimitive();											//结束图元装配
}