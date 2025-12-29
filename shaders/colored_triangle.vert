#version 450

// 输出给片段着色器的颜色
layout (location = 0) out vec3 outColor;

void main() {
    // 硬编码的三角形顶点数组
    // const array of positions for the triangle
    const vec3 positions[3] = vec3[3](
        vec3(1.0, 1.0, 0.0),   // 右下
        vec3(-1.0, 1.0, 0.0),  // 左下
        vec3(0.0, -1.0, 0.0)   // 顶中
    );

    // 对应顶点的颜色
    const vec3 colors[3] = vec3[3](
        vec3(1.0, 0.0, 0.0), // 红
        vec3(0.0, 1.0, 0.0), // 绿
        vec3(0.0, 0.0, 1.0)  // 蓝
    );

    // gl_VertexIndex 是 Vulkan 提供的内置变量，告诉我们当前处理的是第几个点
    gl_Position = vec4(positions[gl_VertexIndex], 1.0f);
    outColor = colors[gl_VertexIndex];
}