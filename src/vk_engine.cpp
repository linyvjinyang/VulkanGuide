#define VMA_IMPLEMENTATION // 启用 VMA 实现

#include "vk_engine.h"// 包含 Vulkan 引擎的头文件
#include "vk_initializers.h"// 包含我们自定义的初始化辅助函数

#include<fstream>
// 引入 SDL
// 这里的路径依赖于我们刚才 CMake 的 include 目录设置
// 如果报错找不到，试试 <SDL2/SDL.h>
#include <SDL.h>// SDL 主头文件
#include <SDL_vulkan.h>// SDL 的 Vulkan 扩展

#include <cmath>// 数学库
#include <glm/gtx/transform.hpp>// GLM 变换扩展

#include <VkBootstrap.h>// 引入 vk-bootstrap，简化 Vulkan 初始化


VkPipeline PipelineBuilder::build_pipeline(VkDevice device) {// 根据配置构建管线
    // 1. 设置视口状态 (Viewport State)
    // 虽然我们使用动态视口，但这个结构体还是得有，只是指向 nullptr
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext = nullptr;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // 2. 设置颜色混合状态 (Color Blending)
    // 这是一个全局设置，通常只要把 attachment 填进去就行
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext = nullptr;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &_colorBlendAttachment;

    // 3. 设置动态状态 (Dynamic State) [关键!]
    // 允许我们在绘制时改变视口大小，而不用重建管线
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicInfo = {};
    dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicInfo.dynamicStateCount = (uint32_t)dynamicStates.size();
    dynamicInfo.pDynamicStates = dynamicStates.data();

    // 4. 组装最终的 CreateInfo
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    // 连接到动态渲染信息 (Vulkan 1.3)
    pipelineInfo.pNext = &_renderInfo;

    pipelineInfo.stageCount = (uint32_t)_shaderStages.size();
    pipelineInfo.pStages = _shaderStages.data();
    pipelineInfo.pVertexInputState = &_vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &_inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &_rasterizer;
    pipelineInfo.pMultisampleState = &_multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &_depthStencil;
    pipelineInfo.layout = _pipelineLayout;
    pipelineInfo.pDynamicState = &dynamicInfo;

    // 5. 真正的创建调用
    VkPipeline newPipeline;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS) {
        std::cout << "[ERROR] Failed to create pipeline" << std::endl;
        return VK_NULL_HANDLE; // 失败返回空句柄
    }
    return newPipeline;
}

void VulkanEngine::init()// 初始化引擎
{
	// 1. 初始化 SDL 视频子系统
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cout << "[ERROR] Could not initialize SDL! Error: " << SDL_GetError() << std::endl;
		return;
	}

	// 2. 创建窗口
	// SDL_WINDOW_VULKAN: 告诉 SDL 我们要用 Vulkan 渲染
	// SDL_WINDOW_RESIZABLE: 允许拖拽改变大小
	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

	_window = SDL_CreateWindow(
		"Vulkan Engine",         // 标题
		SDL_WINDOWPOS_UNDEFINED, // X
		SDL_WINDOWPOS_UNDEFINED, // Y
		_windowExtent.width,     // 宽
		_windowExtent.height,    // 高
		window_flags
	);

	if (!_window) {
		std::cout << "[ERROR] Could not create window! Error: " << SDL_GetError() << std::endl;
		return;
	}

	// ----------------------------------------
	// [新增] 2. 初始化 Vulkan
	// ----------------------------------------
	

	_isInitialized = true;
	std::cout << "[INFO] SDL Initialized & Window Created!" << std::endl;

	init_vulkan();

		// 初始化 VMA 分配器
	VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = _chosenGPU;
    allocatorInfo.device = _device;
    allocatorInfo.instance = _instance;
    // 告诉 VMA 我们用的是 Vulkan 1.3 的一些特性（比如 buffer device address）
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    vmaCreateAllocator(&allocatorInfo, &_allocator);// 创建 VMA 分配器

    std::cout << "[INFO] Vulkan Memory Allocator Initialized!" << std::endl;

	init_swapchain();

	_isInitialized = true;
	std::cout << "[INFO] Engine Initialized Successfully" << std::endl;

	init_commands();      
    init_sync_structures();

    _isInitialized = true;
    std::cout << "[INFO] Engine Fully Initialized!" << std::endl;
	init_default_data();
	init_pipelines();// 初始化管线
    _isInitialized = true;
    std::cout << "[INFO] Pipelines Initialized!" << std::endl;
	

}

// [新增] 实现 Vulkan 初始化逻辑
void VulkanEngine::init_vulkan()// 初始化 Vulkan
{
	// 1. 创建 Instance (实例)
	// vkb::InstanceBuilder 是一个“建造者模式”的工具，帮我们配置参数
	vkb::InstanceBuilder builder;

	auto inst_ret = builder.set_app_name("Example Vulkan Application")
		.request_validation_layers(true) // [重要] 开启验证层！这是新手救星
		.require_api_version(1, 3, 0)    // 我们使用 Vulkan 1.3
		.use_default_debug_messenger()   // 自动把报错打印到控制台
		.build();

	// 检查 Instance 是否创建成功
	vkb::Instance vkb_inst = inst_ret.value();

	// 保存 Instance 句柄
	_instance = vkb_inst.instance;
	_debug_messenger = vkb_inst.debug_messenger;

	// 2. 创建 Surface (表面)
	// SDL 帮我们处理了不同操作系统（Windows/Linux）的细节
	SDL_Vulkan_CreateSurface(_window, _instance, &_surface);

	// 3. 选择 GPU (物理设备)
	// vkb::PhysicalDeviceSelector 会帮我们找到最强的一张显卡
	vkb::PhysicalDeviceSelector selector{ vkb_inst };
	vkb::PhysicalDevice physicalDevice = selector
		.set_minimum_version(1, 3)     // 显卡必须支持 Vulkan 1.3
		.set_surface(_surface)         // 显卡必须能画到这个窗口上
		.select()
		.value();

	// 4. 创建 Device (逻辑设备)
	vkb::DeviceBuilder deviceBuilder{ physicalDevice };
	vkb::Device vkbDevice = deviceBuilder.build().value();

	// 保存设备句柄
	_device = vkbDevice.device;
	_chosenGPU = physicalDevice.physical_device;

	// 从 vkbDevice 中获取图形队列 (Graphics Queue)
    // 显卡可能有专门计算的队列、专门传输的队列，我们需要 "Graphics" 的
    _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    _graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

	

	std::cout << "[INFO] Vulkan Device Initialized!" << std::endl;
	std::cout << "[INFO] GPU: " << physicalDevice.name << std::endl;
}

// 在 init_vulkan 之后添加这个函数
void VulkanEngine::init_swapchain()// 初始化交换链
{
	vkb::SwapchainBuilder swapchainBuilder{_chosenGPU, _device, _surface };

	vkb::Swapchain vkbSwapchain = swapchainBuilder
		.use_default_format_selection() // 自动选择最佳格式 (通常是 B8G8R8A8_SRGB)
		// 呈现模式 (Present Mode): 
		// MAILBOX 是最理想的 (三重缓冲，无撕裂，低延迟)，如果没有就回退到 FIFO (传统垂直同步)
		.set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR) 
		.set_desired_extent(_windowExtent.width, _windowExtent.height)
		.build()
		.value();

	// 1. 保存交换链句柄
	_swapchain = vkbSwapchain.swapchain;
	_swapchainImageFormat = vkbSwapchain.image_format;

	// 2. 获取交换链里的图片 (VkImage)
	// vk-bootstrap 帮我们把这些图从驱动里取出来了
	_swapchainImages = vkbSwapchain.get_images().value();
	_swapchainImageViews = vkbSwapchain.get_image_views().value();

	std::cout << "[INFO] Swapchain Initialized!" << std::endl;
	std::cout << "[INFO] Format: " << _swapchainImageFormat << " | Images: " << _swapchainImages.size() << std::endl;
}

void VulkanEngine::init_commands()// 初始化命令系统
{
	// 1. 创建 Command Pool
	// 它的作用是分配 Command Buffer
	VkCommandPoolCreateInfo commandPoolInfo = {};
	commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolInfo.pNext = nullptr;

	// RESET_COMMAND_BUFFER_BIT: 允许我们要能够单独重置某个 Buffer (虽然我们这里只会用一个)
	commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	// 告诉它是给哪个队列家族用的 (Graphics Queue)
	commandPoolInfo.queueFamilyIndex = _graphicsQueueFamily;

	if (vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_commandPool) != VK_SUCCESS) {
		std::cout << "[ERROR] Failed to create Command Pool" << std::endl;
		return; // 实际工程中应该抛异常
	}

	// 2. 分配 Command Buffer
	VkCommandBufferAllocateInfo cmdAllocInfo = {};
	cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdAllocInfo.pNext = nullptr;

	cmdAllocInfo.commandPool = _commandPool; // 从上面那个池子里拿
	cmdAllocInfo.commandBufferCount = 1;     // 我们只需要 1 个
	cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // 主缓冲区 (可以直接提交给队列)

	if (vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_mainCommandBuffer) != VK_SUCCESS) {
		std::cout << "[ERROR] Failed to allocate Command Buffer" << std::endl;
	}

	std::cout << "[INFO] Command Pool & Buffer Created!" << std::endl;
}

void VulkanEngine::init_sync_structures()//	
{
	// 1. 创建 Fence (围栏)
	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.pNext = nullptr;

	// [关键点] SIGNALED_BIT: 
	// 我们希望 Fence 一开始就是 "开启" (Signaled) 状态。
	// 为什么？因为我们的渲染循环第一步就是 "等待 Fence"。
	// 如果一开始是关闭的，程序会永远卡在第一帧等待，因为它还没被 GPU 执行过。
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	if (vkCreateFence(_device, &fenceInfo, nullptr, &_renderFence) != VK_SUCCESS) {
		std::cout << "[ERROR] Failed to create Fence" << std::endl;
	}

	// 2. 创建 Semaphores (信号量)
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreInfo.pNext = nullptr;
	semaphoreInfo.flags = 0;

	if (vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_presentSemaphore) != VK_SUCCESS) {
		std::cout << "[ERROR] Failed to create Present Semaphore" << std::endl;
	}
	if (vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_renderSemaphore) != VK_SUCCESS) {
		std::cout << "[ERROR] Failed to create Render Semaphore" << std::endl;
	}

	std::cout << "[INFO] Sync Structures (Fence/Semaphores) Created!" << std::endl;
}

// 清理函数
void VulkanEngine::cleanup()
{
	if (_isInitialized) {
		vkDeviceWaitIdle(_device);// 确保设备空闲，避免资源正在使用时被销毁
	
		// 销毁管线和布局
        vkDestroyPipelineLayout(_device, _trianglePipelineLayout, nullptr);
        vkDestroyPipeline(_device, _trianglePipeline, nullptr);

		// [新增] 销毁顶点缓冲区
        vmaDestroyBuffer(_allocator, _vertexBuffer._buffer, _vertexBuffer._allocation);

		// 注意销毁顺序：与创建顺序完全相反！
		// 1. 销毁同步原语
        vkDestroyFence(_device, _renderFence, nullptr);
        vkDestroySemaphore(_device, _presentSemaphore, nullptr);
        vkDestroySemaphore(_device, _renderSemaphore, nullptr);

		// 2. 销毁命令池
        // 注意：销毁 Pool 会自动释放它里面所有的 Buffer，所以不需要单独释放 Buffer
		vkDestroyCommandPool(_device, _commandPool, nullptr);
		
		vmaDestroyAllocator(_allocator);// 销毁 VMA 分配器

		// 3. 销毁交换链相关资源
		// 必须先销毁 Swapchain，再销毁 Device，因为 Swapchain 依赖 Device
		vkDestroySwapchainKHR(_device, _swapchain, nullptr);
		// 销毁 ImageViews
		for (int i = 0; i < _swapchainImageViews.size(); i++) {
			vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
		}
		//4. 销毁逻辑设备
		vkDestroyDevice(_device, nullptr);

		// 5. 销毁表面
		vkDestroySurfaceKHR(_instance, _surface, nullptr);

		// 6. 销毁调试信使 (vk-bootstrap 提供的辅助函数)
		vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);

		// 7. 销毁实例
		vkDestroyInstance(_instance, nullptr);

		// 最后销毁窗口
		SDL_DestroyWindow(_window);
		SDL_Quit();
	}
}

void VulkanEngine::draw()
{
	// =================================================================
	// 1. 等待上一帧完成 (CPU 等待 GPU)
	// =================================================================
	// 等待围栏 (Fence) 变为 Signaled 状态。
	// 1000000000 ns = 1秒 (超时时间)
	vkWaitForFences(_device, 1, &_renderFence, true, 1000000000);
	
	// 必须手动重置围栏，将其变回 Unsignaled 状态，以便下一帧使用
	vkResetFences(_device, 1, &_renderFence);

	// =================================================================
	// 2. 获取交换链图片 (请求画布)
	// =================================================================
	
	uint32_t swapchainImageIndex;
	// 询问交换链下一张可用的图片索引。
	// 当图片可用时，显卡会发出信号给 _presentSemaphore (我们不需要在 CPU 端等待)
	vkAcquireNextImageKHR(_device, _swapchain, 1000000000, _presentSemaphore, nullptr, &swapchainImageIndex);

	// =================================================================
	// 3. 记录命令缓冲区 (写清单)
	// =================================================================

	// A. 重置命令缓冲区 (清空旧的指令)
	vkResetCommandBuffer(_mainCommandBuffer, 0);

	// B. 开始记录
	VkCommandBufferBeginInfo cmdBeginInfo = {};
	cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;// 结构体类型
	cmdBeginInfo.pNext = nullptr;// 无扩展
	cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;//	 我们只会用这次
	vkBeginCommandBuffer(_mainCommandBuffer, &cmdBeginInfo);// 开始记录

	// --- [关键步骤] 图片布局转换 (Layout Transition) ---
	// 图片刚拿来时是 "Undefined" 状态，或者是上次呈现后的 "Present" 状态。
	// 我们必须把它变成 "Color Attachment" (可绘制) 状态才能往上画画。
	
	VkImageMemoryBarrier imgBarrier = {};
	imgBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imgBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; // 我们不关心它之前是什么
	imgBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // 目标：最佳绘制状态
	imgBarrier.image = _swapchainImages[swapchainImageIndex]; // 指定要转换哪张图
	imgBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;// 颜色图
	imgBarrier.subresourceRange.baseMipLevel = 0;// 从第 0 层开始
	imgBarrier.subresourceRange.levelCount = 1;// 仅第 0 层
	imgBarrier.subresourceRange.baseArrayLayer = 0;// 从第 0 层开始
	imgBarrier.subresourceRange.layerCount = 1;// 仅一层
	imgBarrier.srcAccessMask = 0;// 之前不需要等待任何操作
	imgBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;// 之后要等待写入颜色

	// 这是一个 Pipeline Barrier，用于在 GPU 内部协调流水线阶段
	vkCmdPipelineBarrier(
		_mainCommandBuffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // 来源阶段：一开始
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // 目标阶段：输出颜色时
		0,
		0, nullptr,
		0, nullptr,
		1, &imgBarrier // 也就是上面的那个 barrier
	);

	// --- [动态渲染] 开始画画 (Dynamic Rendering) ---
	
	// 计算一个闪烁的颜色 (根据帧数 frameNumber)
	float flash = std::abs(std::sin(_frameNumber / 120.f));
	VkClearValue clearValue = { { 0.0f, 0.0f, flash, 1.0f } }; // 蓝色通道闪烁
	VkRenderingAttachmentInfo colorAttachment = {};
	colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	colorAttachment.imageView = _swapchainImageViews[swapchainImageIndex];
	colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // 必须匹配上面的转换
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // 加载时：清屏
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // 结束后：保存结果
	colorAttachment.clearValue = clearValue;

	VkRenderingInfo renderInfo = {};
	renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderInfo.renderArea = { 0, 0, _windowExtent.width, _windowExtent.height };
	renderInfo.layerCount = 1;
	renderInfo.colorAttachmentCount = 1;
	renderInfo.pColorAttachments = &colorAttachment;

	// 开始动态渲染 (Vulkan 1.3 核心功能)
	vkCmdBeginRendering(_mainCommandBuffer, &renderInfo);

	// 1. 绑定管线
    vkCmdBindPipeline(_mainCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _trianglePipeline);// 绑定三角形管线

    // 2. 设置动态视口 (Dynamic Viewport)
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)_windowExtent.width;
    viewport.height = (float)_windowExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(_mainCommandBuffer, 0, 1, &viewport);

    // 3. 设置动态剪裁 (Dynamic Scissor)
    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = _windowExtent;
    vkCmdSetScissor(_mainCommandBuffer, 0, 1, &scissor);

	// [新增] 2. 绑定顶点缓冲区 (Bind Vertex Buffer)
    VkDeviceSize offset = 0;
    // 绑定到 0 号槽位，使用 _vertexBuffer._buffer
    vkCmdBindVertexBuffers(_mainCommandBuffer, 0, 1, &_vertexBuffer._buffer, &offset);

	// 1. 创建一个简单的摄像机位置
    glm::vec3 camPos = { 0.f, 0.f, -10.f }; // 往后拉一点，这样能看到原点
    glm::mat4 view = glm::translate(glm::mat4(1.f), camPos);
    
    // 2. 创建透视投影矩阵 (Perspective Projection)
    // fov=70度, 宽高比=窗口宽高比, 近平面=0.1, 远平面=200
    glm::mat4 projection = glm::perspective(glm::radians(70.f), (float)_windowExtent.width / (float)_windowExtent.height, 0.1f, 200.0f);
    
    // [修正] GLM 的 Y 轴是向上的，Vulkan 的 Y 轴是向下的。
    // 我们把 Y 轴翻转一下，否则画面是倒的
    projection[1][1] *= -1;

    // 3. 计算模型矩阵 (Model Matrix) - 让它自转
    glm::mat4 model = glm::rotate(glm::mat4(1.f), glm::radians(_frameNumber * 0.4f), glm::vec3(0, 1, 0));

    // 4. 最终矩阵 = 投影 * 视图 * 模型
    glm::mat4 meshMatrix = projection * view * model;

    // 5. 定义 PushConstant 数据
    MeshPushConstants constants;
    constants.render_matrix = meshMatrix;

    // 6. 发送 Push Constants!
    vkCmdPushConstants(_mainCommandBuffer, _trianglePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);

    // =============================================================
    // 4. 绘制！
	//vkCmdDraw(_mainCommandBuffer, 3, 1, 0, 0);
	// 绘制立方体，共 36 个顶点 (6 个面 * 2 个三角形 * 3 个顶点)
    vkCmdDraw(_mainCommandBuffer, 36, 1, 0, 0);
	
	vkCmdEndRendering(_mainCommandBuffer);// 结束动态渲染

	// --- [关键步骤] 图片布局转换 (变回去) ---
	// 画完了，现在要把图片变成 "Present Src" (最佳呈现状态)，以便显示器读取
	
	imgBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	imgBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	imgBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	imgBarrier.dstAccessMask = 0;

	vkCmdPipelineBarrier(
		_mainCommandBuffer,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &imgBarrier
	);

	// 结束记录
	vkEndCommandBuffer(_mainCommandBuffer);

	// =================================================================
	// 4. 提交给 GPU (Execute)
	// =================================================================

	VkSubmitInfo submit = {};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	// 等待信号量：_presentSemaphore (等交换链把图给我们)
	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submit.pWaitDstStageMask = &waitStage;
	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &_presentSemaphore;

	// 提交命令缓冲区
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &_mainCommandBuffer;

	// 完成信号量：_renderSemaphore (画完了通知交换链)
	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &_renderSemaphore;

	// 提交！并把 _renderFence 传进去，这样 CPU 就能知道这帧什么时候算完
	if (vkQueueSubmit(_graphicsQueue, 1, &submit, _renderFence) != VK_SUCCESS) {
		std::cout << "[ERROR] Failed to submit draw command buffer!" << std::endl;
	}

	// =================================================================
	// 5. 呈现 (Present)
	// =================================================================
	
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &_swapchain;
	
	// 等待信号量：_renderSemaphore (等显卡画完)
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &_renderSemaphore;
	
	presentInfo.pImageIndices = &swapchainImageIndex;

	vkQueuePresentKHR(_graphicsQueue, &presentInfo);

	// 帧数加一
	_frameNumber++;
}

void VulkanEngine::run()
{
	SDL_Event e;
	bool bQuit = false;

	// 无限循环 (Game Loop)
	while (!bQuit)
	{
		// 处理事件队列
		while (SDL_PollEvent(&e) != 0)
		{
			// 如果点击了关闭按钮 (X)
			if (e.type == SDL_QUIT) {
				bQuit = true;
			}
			
			// 如果按下了 ESC 键
			if (e.type == SDL_KEYDOWN) {
				if (e.key.keysym.sym == SDLK_ESCAPE) {
					bQuit = true;
				}
			}
		}

		// 只有在没最小化时才绘制
		draw();
	}
}

bool VulkanEngine::load_shader_module(const char* filePath, VkShaderModule* outShaderModule)
{
	// 1. 以二进制模式打开文件，并将指针移到文件末尾 (ate)
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		return false;
	}

	// 2. 因为指针在末尾，tellg() 可以告诉我们文件有多大
	size_t fileSize = (size_t)file.tellg();

	// 3. 分配缓冲区 (uint32_t 是 SPIR-V 的对齐要求)
	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

	// 4. 指针移回开头，开始读取
	file.seekg(0);
	file.read((char*)buffer.data(), fileSize);

	file.close();

	// 5. 创建 Vulkan Shader Module
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pNext = nullptr;

	// codeSize 必须是字节数
	createInfo.codeSize = buffer.size() * sizeof(uint32_t);
	// pCode 指向 uint32_t 数组
	createInfo.pCode = buffer.data();

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		return false;
	}

	*outShaderModule = shaderModule;
	return true;
}

void VulkanEngine::init_pipelines()// 初始化管线
{
  
  VkShaderModule triangleVertexShader;
    if (!load_shader_module("shaders/triangle_mesh.vert.spv", &triangleVertexShader)) {
        std::cout << "[ERROR] Failed to load triangle_mesh.vert.spv" << std::endl;
    }
	else {
		std::cout << "[INFO] Triangle mesh vertex shader successfully loaded" << std::endl;
	}

	 VkShaderModule triangleFragShader;
    if (!load_shader_module("shaders/colored_triangle.frag.spv", &triangleFragShader)) {
        std::cout << "[ERROR] Error when building the triangle fragment shader module" << std::endl;
    }
    else {
        std::cout << "[INFO] Triangle vertex shader successfully loaded" << std::endl;
    }

	// [新增] 1. 配置 Push Constant Range
    VkPushConstantRange pushConstantRange = {};// 推送常量范围
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(MeshPushConstants);

	// 告诉 Vulkan 这个数据只在 Vertex Shader 里用
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;// 推送常量阶段标志

    // 1. 创建 Pipeline Layout (管线布局)
    //  管线布局里告诉 Vulkan 我们会用到哪些资源 (Descriptor Sets) 和 Push Constants
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipeline_layout_create_info();// 管线布局创建信息
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pNext = nullptr;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 1;// 我们有 1 个 Push Constant Range
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;// 指向上面定义的 Push Constant Range

    if (vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &_trianglePipelineLayout) != VK_SUCCESS) {
        std::cout << "[ERROR] Failed to create pipeline layout" << std::endl;
    }

    // 2. 开始构建 Pipeline
    PipelineBuilder pipelineBuilder;

    // -- A. Shader Stages --
    pipelineBuilder._shaderStages.push_back(
        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, triangleVertexShader));
    pipelineBuilder._shaderStages.push_back(
        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, triangleFragShader));

    // -- B. Vertex Input (空) --
    // 我们目前把顶点硬编码在 Shader 里，所以这里不需要绑定任何 Buffer
	VkVertexInputBindingDescription bindingDescription = Vertex::get_binding_description();
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions = Vertex::get_attribute_descriptions();

    // 连接到 Pipeline Builder
    pipelineBuilder._vertexInputInfo = vkinit::pipeline_vertex_input_state_create_info();
    
    pipelineBuilder._vertexInputInfo.vertexBindingDescriptionCount = 1;
    pipelineBuilder._vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    
    pipelineBuilder._vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)attributeDescriptions.size();
    pipelineBuilder._vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    // -- C. Input Assembly (三角形列表) --
    pipelineBuilder._inputAssembly = vkinit::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    // -- D. Rasterizer (光栅化) --
    // 多边形模式：FILL (填满), CULL_MODE_NONE (不剔除背面), CCW (逆时针为正面)
    pipelineBuilder._rasterizer = vkinit::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL);// 光栅化状态创建信息
	pipelineBuilder._rasterizer.cullMode = VK_CULL_MODE_NONE;// 不剔除背面
    pipelineBuilder._rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

    // -- E. Multisampling (关闭) --
    pipelineBuilder._multisampling = vkinit::pipeline_multisample_state_create_info();

    // -- F. Color Blend (关闭混合) --
    pipelineBuilder._colorBlendAttachment = vkinit::pipeline_color_blend_attachment_state();

    // -- G. Depth Stencil (关闭深度测试) --
    pipelineBuilder._depthStencil = vkinit::pipeline_depth_stencil_state_create_info(false, false, VK_COMPARE_OP_ALWAYS);

    // -- H. Rendering Info (动态渲染) --
    // 这里非常关键！告诉管线我们要画到什么格式的图片上
    pipelineBuilder._renderInfo = {};
    pipelineBuilder._renderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineBuilder._renderInfo.colorAttachmentCount = 1;
    pipelineBuilder._renderInfo.pColorAttachmentFormats = &_swapchainImageFormat;
    pipelineBuilder._renderInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED; // 暂时没有深度缓冲

    // -- I. 赋予 Layout --
    pipelineBuilder._pipelineLayout = _trianglePipelineLayout;

    // 3. 最终构建
    _trianglePipeline = pipelineBuilder.build_pipeline(_device);
    
    // 4. 清理 Shader Module
    // 管线创建好后，Shader Module 就可以丢掉了，因为代码已经被拷贝到管线里了
    vkDestroyShaderModule(_device, triangleFragShader, nullptr);
    vkDestroyShaderModule(_device, triangleVertexShader, nullptr);

    std::cout << "[INFO] Triangle Pipeline Created Successfully!" << std::endl;
}

// 1. Buffer 创建助手
AllocatedBuffer VulkanEngine::create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)// 创建 Buffer 的辅助函数
{
	// 填写 Buffer 创建信息
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.pNext = nullptr;
	bufferInfo.size = allocSize;
	bufferInfo.usage = usage;

	// 填写 VMA 分配信息
	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = memoryUsage;

	AllocatedBuffer newBuffer;

	// 让 VMA 帮我们申请 Buffer 和显存，并绑定好
	if (vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo,
		&newBuffer._buffer,
		&newBuffer._allocation,
		nullptr) != VK_SUCCESS)
	{
		std::cout << "[ERROR] Failed to allocate buffer!" << std::endl;
	}

	return newBuffer;
}

// 2. 上传立方体数据
void VulkanEngine::init_default_data()// 初始化默认数据
{
	// 立方体的 8 个角点颜色
    glm::vec3 white = {1.f, 1.f, 1.f};
    glm::vec3 red   = {1.f, 0.f, 0.f};
    glm::vec3 green = {0.f, 1.f, 0.f};
    glm::vec3 blue  = {0.f, 0.f, 1.f};

    // 顶点数组：每个面由两个三角形组成，共 6 面 * 2 三角 * 3 点 = 36 个点
    std::vector<Vertex> vertices = {
        // --- 前面 (Z+) ---
        // pos                  // uv   // norm   // uv   // color
        {{-0.5f, -0.5f,  0.5f}, 0.f, {0,0,1}, 0.f, red},
        {{ 0.5f, -0.5f,  0.5f}, 0.f, {0,0,1}, 0.f, red},
        {{ 0.5f,  0.5f,  0.5f}, 0.f, {0,0,1}, 0.f, red},
        {{ 0.5f,  0.5f,  0.5f}, 0.f, {0,0,1}, 0.f, red},
        {{-0.5f,  0.5f,  0.5f}, 0.f, {0,0,1}, 0.f, red},
        {{-0.5f, -0.5f,  0.5f}, 0.f, {0,0,1}, 0.f, red},

        // --- 后面 (Z-) ---
        {{ 0.5f, -0.5f, -0.5f}, 0.f, {0,0,-1}, 0.f, green},
        {{-0.5f, -0.5f, -0.5f}, 0.f, {0,0,-1}, 0.f, green},
        {{-0.5f,  0.5f, -0.5f}, 0.f, {0,0,-1}, 0.f, green},
        {{-0.5f,  0.5f, -0.5f}, 0.f, {0,0,-1}, 0.f, green},
        {{ 0.5f,  0.5f, -0.5f}, 0.f, {0,0,-1}, 0.f, green},
        {{ 0.5f, -0.5f, -0.5f}, 0.f, {0,0,-1}, 0.f, green},

        // --- 顶面 (Y-) ---
        {{-0.5f, -0.5f, -0.5f}, 0.f, {0,-1,0}, 0.f, blue},
        {{ 0.5f, -0.5f, -0.5f}, 0.f, {0,-1,0}, 0.f, blue},
        {{ 0.5f, -0.5f,  0.5f}, 0.f, {0,-1,0}, 0.f, blue},
        {{ 0.5f, -0.5f,  0.5f}, 0.f, {0,-1,0}, 0.f, blue},
        {{-0.5f, -0.5f,  0.5f}, 0.f, {0,-1,0}, 0.f, blue},
        {{-0.5f, -0.5f, -0.5f}, 0.f, {0,-1,0}, 0.f, blue},

        // --- 底面 (Y+) ---
        {{-0.5f,  0.5f,  0.5f}, 0.f, {0,1,0}, 0.f, white},
        {{ 0.5f,  0.5f,  0.5f}, 0.f, {0,1,0}, 0.f, white},
        {{ 0.5f,  0.5f, -0.5f}, 0.f, {0,1,0}, 0.f, white},
        {{ 0.5f,  0.5f, -0.5f}, 0.f, {0,1,0}, 0.f, white},
        {{-0.5f,  0.5f, -0.5f}, 0.f, {0,1,0}, 0.f, white},
        {{-0.5f,  0.5f,  0.5f}, 0.f, {0,1,0}, 0.f, white},

        // --- 左面 (X-) ---
        {{-0.5f, -0.5f, -0.5f}, 0.f, {-1,0,0}, 0.f, white},
        {{-0.5f, -0.5f,  0.5f}, 0.f, {-1,0,0}, 0.f, white},
        {{-0.5f,  0.5f,  0.5f}, 0.f, {-1,0,0}, 0.f, white},
        {{-0.5f,  0.5f,  0.5f}, 0.f, {-1,0,0}, 0.f, white},
        {{-0.5f,  0.5f, -0.5f}, 0.f, {-1,0,0}, 0.f, white},
        {{-0.5f, -0.5f, -0.5f}, 0.f, {-1,0,0}, 0.f, white},

        // --- 右面 (X+) ---
        {{ 0.5f, -0.5f,  0.5f}, 0.f, {1,0,0}, 0.f, blue},
        {{ 0.5f, -0.5f, -0.5f}, 0.f, {1,0,0}, 0.f, blue},
        {{ 0.5f,  0.5f, -0.5f}, 0.f, {1,0,0}, 0.f, blue},
        {{ 0.5f,  0.5f, -0.5f}, 0.f, {1,0,0}, 0.f, blue},
        {{ 0.5f,  0.5f,  0.5f}, 0.f, {1,0,0}, 0.f, blue},
        {{ 0.5f, -0.5f,  0.5f}, 0.f, {1,0,0}, 0.f, blue},
    };

    // 计算总大小
    const size_t bufferSize = vertices.size() * sizeof(Vertex);

    // 创建 Buffer (和之前一样)
    _vertexBuffer = create_buffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    // 拷贝数据 (和之前一样)
    void* data;
    vmaMapMemory(_allocator, _vertexBuffer._allocation, &data);
    memcpy(data, vertices.data(), bufferSize);
    vmaUnmapMemory(_allocator, _vertexBuffer._allocation);

    std::cout << "[INFO] Cube Mesh Uploaded!" << std::endl;
}