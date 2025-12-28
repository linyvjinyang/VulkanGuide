#pragma once

#include "vk_types.h"

struct SDL_Window;
union SDL_Event;

class VulkanEngine {
public:
	bool _isInitialized{ false };
	int _frameNumber {0};
	bool _stop_rendering{ false };
	VkExtent2D _windowExtent{ 1700 , 900 };

	struct SDL_Window* _window{ nullptr };

	// ----- 新增：Vulkan 核心句柄 -----
	VkInstance _instance;                      // Vulkan 实例
	VkDebugUtilsMessengerEXT _debug_messenger; // 调试信使（用于接收报错）
	VkPhysicalDevice _chosenGPU;               // 也就是物理设备 (GPU)
	VkDevice _device;                          // 逻辑设备 (驱动接口)
	VkSurfaceKHR _surface;                     // 渲染表面 (窗口)
	// -------------------------------

	// [新增] --- 交换链相关 ---
	VkSwapchainKHR _swapchain;           // 交换链句柄
	VkFormat _swapchainImageFormat;      // 图片格式 (比如: 蓝色-绿色-红色-透明度)
	std::vector<VkImage> _swapchainImages; // 实际的图片数组 (通常是3张)
	std::vector<VkImageView> _swapchainImageViews; // 图片视图 (这是我们要手动创建的)


	// 初始化三部曲
	void init();
	void cleanup();
	void run();
	void draw();

private:
	// ----- 新增：初始化 Vulkan 的私有函数 -----
	void init_vulkan(); 
	void init_swapchain(); // [新增] 初始化交换链函数
};