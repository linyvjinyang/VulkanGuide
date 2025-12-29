#pragma once

#include "vk_types.h"

struct SDL_Window;
union SDL_Event;

class PipelineBuilder {// 用于构建图形管线的辅助类
public:
	std::vector<VkPipelineShaderStageCreateInfo> _shaderStages; // 着色器阶段
	VkPipelineVertexInputStateCreateInfo _vertexInputInfo;      // 顶点输入格式
	VkPipelineInputAssemblyStateCreateInfo _inputAssembly;      // 图元装配 (三角形/线/点)
	VkViewport _viewport;                                       // 视口 (废弃，我们用动态视口)
	VkRect2D _scissor;                                          // 剪裁 (废弃，我们用动态视口)
	VkPipelineRasterizationStateCreateInfo _rasterizer;         // 光栅化设置
	VkPipelineColorBlendAttachmentState _colorBlendAttachment;  // 颜色混合
	VkPipelineMultisampleStateCreateInfo _multisampling;        // 多重采样
	VkPipelineLayout _pipelineLayout;                           // 管线布局 (Push Constants 等)
    VkPipelineDepthStencilStateCreateInfo _depthStencil;        // 深度测试
    VkPipelineRenderingCreateInfo _renderInfo;                  // 动态渲染信息
    VkFormat _colorAttachmentformat;                            // 颜色格式

	// 核心函数：根据以上配置构建管线
	VkPipeline build_pipeline(VkDevice device);
};

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

	//  --- 交换链相关 ---
	VkSwapchainKHR _swapchain;           // 交换链句柄
	VkFormat _swapchainImageFormat;      // 图片格式 (比如: 蓝色-绿色-红色-透明度)
	std::vector<VkImage> _swapchainImages; // 实际的图片数组 (通常是3张)
	std::vector<VkImageView> _swapchainImageViews; // 图片视图 (这是我们要手动创建的)

	//  --- 命令与同步 ---
	
	VkQueue _graphicsQueue;        // 图形队列 (提交命令的地方)
	uint32_t _graphicsQueueFamily; // 队列家族索引 (显卡有很多种队列，我们要找能画图的那种)

	VkCommandPool _commandPool;    // 命令池 (分配器)
	VkCommandBuffer _mainCommandBuffer; // 主命令缓冲区 (我们会把每一帧的指令录在这里)

	VkFence _renderFence;          // 围栏: 确保 CPU 不会跑得比 GPU 快太多
	VkSemaphore _presentSemaphore; // 信号量: 图片准备好了吗？
	VkSemaphore _renderSemaphore;  // 信号量: 画完了吗？

	VmaAllocator _allocator; // VMA 分配器

	// 初始化三部曲
	void init();
	void cleanup();
	void run();
	void draw();

	// 三角形相关
	VkPipelineLayout _trianglePipelineLayout;// 三角形管线布局
    VkPipeline _trianglePipeline;// 三角形管线

private:
	// ----- 新增：初始化 Vulkan 的私有函数 -----
	void init_vulkan(); 
	void init_swapchain(); //  初始化交换链函数
	void init_commands(); //  初始化命令系统
	void init_sync_structures(); // 初始化同步原语

	bool load_shader_module(const char* filePath, VkShaderModule* outShaderModule);// 加载着色器模块

	void init_pipelines();// 初始化管线
};