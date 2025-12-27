#include "vk_engine.h"

int main(int argc, char* argv[])
{
	VulkanEngine engine;

	// 1. 初始化 (弹窗)
	engine.init();	

	// 2. 运行 (卡在这里循环)
	engine.run();	

	// 3. 清理 (关闭)
	engine.cleanup();	

	return 0;
}