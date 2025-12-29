#version 450

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColor;

layout (location = 0) out vec3 outColor;

// [新增] Push Constants 定义
// 这就像是一个全局变量，由 C++ 直接塞进来
layout(push_constant) uniform PushConstants {
	mat4 renderMatrix; // 渲染矩阵 (模型+视图+投影)
} pushConstants;

void main()
{
	// [修改] 使用矩阵变换顶点位置
	// 注意矩阵乘法的顺序：矩阵 * 向量
	gl_Position = pushConstants.renderMatrix * vec4(vPosition, 1.0f);
	outColor = vColor;
}