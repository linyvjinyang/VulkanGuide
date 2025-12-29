#version 450

// 对应 Vertex 结构体:
// layout(location = 0) in vec3 position;
// layout(location = 1) in vec3 normal; (暂时不用)
// layout(location = 2) in vec3 color;

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal; // 注意 Vertex 结构体里还有 UV 占位，这里要注意对齐
layout (location = 2) in vec3 vColor;

layout (location = 0) out vec3 outColor;

void main()
{
	gl_Position = vec4(vPosition, 1.0f);
	outColor = vColor;
}