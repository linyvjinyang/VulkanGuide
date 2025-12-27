#include <vulkan/vulkan.h>
#include <iostream>

int main(int argc, char* argv[]) {
    // 仅仅是初始化一个 Instance Info，看看能不能编译通过
    VkInstanceCreateInfo instanceInfo{};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    std::cout << "Vulkan Environment is ready!" << std::endl;
    
    // 我们不需要真的创建 Instance，只要编译不报错，
    // 就说明 CMake 成功找到了 Vulkan SDK。
    
    return 0;
}