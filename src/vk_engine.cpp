#include "vk_engine.h"

// 引入 SDL
// 这里的路径依赖于我们刚才 CMake 的 include 目录设置
// 如果报错找不到，试试 <SDL2/SDL.h>
#include <SDL.h>
#include <SDL_vulkan.h>

#include <iostream>

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

	_isInitialized = true;
	std::cout << "[INFO] SDL Initialized & Window Created!" << std::endl;
}

void VulkanEngine::cleanup()
{
	if (_isInitialized) {
		// 销毁窗口
		SDL_DestroyWindow(_window);
		// 退出 SDL
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