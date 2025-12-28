#include "vk_engine.h"

// 引入 SDL
// 这里的路径依赖于我们刚才 CMake 的 include 目录设置
// 如果报错找不到，试试 <SDL2/SDL.h>
#include <SDL.h>
#include <SDL_vulkan.h>

#include <iostream>

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

// 清理函数
void VulkanEngine::cleanup()
{
	if (_isInitialized) {
		// [新增] 销毁 Vulkan 资源
		// 注意销毁顺序：与创建顺序完全相反！
		// 必须先销毁 Swapchain，再销毁 Device，因为 Swapchain 依赖 Device
		vkDestroySwapchainKHR(_device, _swapchain, nullptr);
		// 销毁 ImageViews
		for (int i = 0; i < _swapchainImageViews.size(); i++) {
			vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
		}
		// 1. 销毁逻辑设备
		vkDestroyDevice(_device, nullptr);

		// 2. 销毁表面
		vkDestroySurfaceKHR(_instance, _surface, nullptr);

		// 3. 销毁调试信使 (vk-bootstrap 提供的辅助函数)
		vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);

		// 4. 销毁实例
		vkDestroyInstance(_instance, nullptr);

		// 最后销毁窗口
		SDL_DestroyWindow(_window);
		SDL_Quit();
	}
}

void VulkanEngine::draw()
{
	// 暂时留空，下个章节填入 Vulkan 渲染代码
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