#version 450

// 从顶点着色器接收的颜色 (插值后)
layout (location = 0) in vec3 inColor;

// 输出到屏幕的颜色
layout (location = 0) out vec4 outFragColor;

void main() {
    // 输出颜色，Alpha 设为 1.0
    outFragColor = vec4(inColor, 1.0f);
}