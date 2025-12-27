#pragma once

#include "vk_types.h"

// 前置声明 (Forward declaration)，避免在头文件中包含巨大的 SDL.h
struct SDL_Window;
union SDL_Event;

class VulkanEngine {
public:
	bool _isInitialized{ false };
	int _frameNumber {0};
	bool _stop_rendering{ false };
	VkExtent2D _windowExtent{ 1700 , 900 };

	struct SDL_Window* _window{ nullptr }; // 我们的窗口句柄

	// --- 核心函数 ---
	void init();    // 初始化
	void cleanup(); // 销毁
	void run();     // 主循环
	void draw();    // 绘制循环
};