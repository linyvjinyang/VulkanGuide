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

    // [新增] 1. 描述“每读取一个点，指针要移动多少步” (Stride)
    static VkVertexInputBindingDescription get_binding_description() {
        VkVertexInputBindingDescription description = {};
        description.binding = 0; // 我们只用一个 Buffer，绑定到 0 号槽位
        description.stride = sizeof(Vertex);
        description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // 每处理一个顶点移动一次
        return description;
    }

    // [新增] 2. 描述“每个属性在结构体里的哪里” (Attribute)
    static std::vector<VkVertexInputAttributeDescription> get_attribute_descriptions() {
        std::vector<VkVertexInputAttributeDescription> attributes;
        attributes.resize(3); // 我们目前Shader里用了 Location 0, 1, 2

        // Location 0: Position
        attributes[0].binding = 0;
        attributes[0].location = 0;
        attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
        attributes[0].offset = offsetof(Vertex, position);

        // Location 1: Normal (Shader里定义了，虽然我们还没用，但必须匹配)
        attributes[1].binding = 0;
        attributes[1].location = 1;
        attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
        attributes[1].offset = offsetof(Vertex, normal);

        // Location 2: Color
        attributes[2].binding = 0;
        attributes[2].location = 2;
        attributes[2].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
        attributes[2].offset = offsetof(Vertex, color);

        return attributes;
    }
};