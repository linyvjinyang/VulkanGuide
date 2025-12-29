#pragma once

#include "vk_types.h"

namespace vkinit {

	// 1. 命令缓冲区分配信息
	VkCommandBufferAllocateInfo command_buffer_allocate_info(VkCommandPool pool, uint32_t count = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	// 2. 围栏创建信息
	VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags = 0);

	// 3. 信号量创建信息
	VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags = 0);

	// 4. 着色器阶段创建信息
	VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule shaderModule);

	// 5. 顶点输入状态 (暂时为空，因为我们在 Shader 里硬编码了三角形)
	VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info();

	// 6. 图元装配 (告诉它是三角形还是线)
	VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info(VkPrimitiveTopology topology);

	// 7. 光栅化设置 (实心/线框，剔除背面等)
	VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info(VkPolygonMode polygonMode);

	// 8. 多重采样 (MSAA)
	VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info();

	// 9. 颜色混合 (透明度混合)
	VkPipelineColorBlendAttachmentState pipeline_color_blend_attachment_state();

	// 10. 管线布局 (Push Constants)
	VkPipelineLayoutCreateInfo pipeline_layout_create_info();

	// 11. 深度测试设置
	VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info(bool bDepthTest, bool bDepthWrite, VkCompareOp compareOp);
}