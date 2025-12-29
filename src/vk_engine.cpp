#include "vk_engine.h"

// 引入 SDL
// 这里的路径依赖于我们刚才 CMake 的 include 目录设置
// 如果报错找不到，试试 <SDL2/SDL.h>
#include <SDL.h>
#include <SDL_vulkan.h>

#include <cmath>

#include <VkBootstrap.h>


void VulkanEngine::init()
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
	init_swapchain();

	_isInitialized = true;
	std::cout << "[INFO] Engine Initialized Successfully" << std::endl;

	init_commands();      
    init_sync_structures();

    _isInitialized = true;
    std::cout << "[INFO] Engine Fully Initialized!" << std::endl;
	
}

// [新增] 实现 Vulkan 初始化逻辑
void VulkanEngine::init_vulkan()
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
void VulkanEngine::init_swapchain()
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

void VulkanEngine::init_commands()
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

void VulkanEngine::init_sync_structures()
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
	
		// 注意销毁顺序：与创建顺序完全相反！
		// 1. 销毁同步原语
        vkDestroyFence(_device, _renderFence, nullptr);
        vkDestroySemaphore(_device, _presentSemaphore, nullptr);
        vkDestroySemaphore(_device, _renderSemaphore, nullptr);

		// 2. 销毁命令池
        // 注意：销毁 Pool 会自动释放它里面所有的 Buffer，所以不需要单独释放 Buffer
		vkDestroyCommandPool(_device, _commandPool, nullptr);
		
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
	cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBeginInfo.pNext = nullptr;
	// ONE_TIME_SUBMIT: 告诉驱动这个 buffer 我们只用一次，下一帧会重录。驱动可以据此优化。
	cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(_mainCommandBuffer, &cmdBeginInfo);

	// --- [关键步骤] 图片布局转换 (Layout Transition) ---
	// 图片刚拿来时是 "Undefined" 状态，或者是上次呈现后的 "Present" 状态。
	// 我们必须把它变成 "Color Attachment" (可绘制) 状态才能往上画画。
	
	VkImageMemoryBarrier imgBarrier = {};
	imgBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imgBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; // 我们不关心它之前是什么
	imgBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // 目标：最佳绘制状态
	imgBarrier.image = _swapchainImages[swapchainImageIndex]; // 指定要转换哪张图
	imgBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imgBarrier.subresourceRange.baseMipLevel = 0;
	imgBarrier.subresourceRange.levelCount = 1;
	imgBarrier.subresourceRange.baseArrayLayer = 0;
	imgBarrier.subresourceRange.layerCount = 1;
	
	// 告诉 GPU：在转换完成前，不仅要阻断像素操作，还要刷新相关的缓存
	imgBarrier.srcAccessMask = 0;
	imgBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

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

	// ==> 这里以后会写画三角形的代码 <==
	
	vkCmdEndRendering(_mainCommandBuffer);

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