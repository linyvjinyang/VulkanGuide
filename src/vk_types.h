#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <functional>
#include <deque>
#include <iostream>

// Vulkan
#include <vulkan/vulkan.h>

// GLM
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

// VMA
#include <vk_mem_alloc.h>


// [新增] 简单的分配缓冲区结构体
// 每次我们用 VMA 分配内存，都会得到一个 VkBuffer (Vulkan句柄) 和 VmaAllocation (VMA句柄)
struct AllocatedBuffer {
    VkBuffer _buffer;
    VmaAllocation _allocation;
};

// [新增] 顶点定义
struct Vertex {
    glm::vec3 position; // 位置
    float uv_x;         // 纹理坐标 X (暂时用来占位，保持对齐)
    glm::vec3 normal;   // 法线
    float uv_y;         // 纹理坐标 Y (暂时用来占位)
    glm::vec3 color;    // 颜色
};