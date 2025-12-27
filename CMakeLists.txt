cmake_minimum_required(VERSION 3.15)

# 项目名称
project(VulkanGuide)

# 使用 C++ 17 标准 (vk-bootstrap 需要这个)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 告诉 CMake 我们要找 Vulkan 包
find_package(Vulkan REQUIRED)

# --- 源代码管理 ---
# 告诉 CMake 我们的源代码都在 src 文件夹里
# file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h") 
# 为了简单起见，我们先手动指定 main.cpp，以后再用上面的自动搜索
set(SOURCES src/main.cpp)

# --- 生成可执行文件 ---
add_executable(VulkanEngine ${SOURCES})

# --- 链接库 ---
# 把 Vulkan 库连接到我们的程序上
target_link_libraries(VulkanEngine PRIVATE Vulkan::Vulkan)

# 如果是 Windows，可能还需要这个来避免控制台的一些问题（可选）
if(WIN32)
    # target_link_libraries(VulkanEngine PRIVATE Vulkan::Vulkan) 
endif()