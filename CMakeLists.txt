cmake_minimum_required(VERSION 3.15)
project(VulkanGuide)

# 1. 设置 C++ 17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 🌟 关键修正：强制 MSVC 编译器使用 UTF-8 ---
# 这行代码能彻底解决 C4819 乱码警告，以后你在 VS Code 里随便写中文/日文注释都没问题！
if(MSVC)
    add_compile_options(/utf-8)
endif()

# 2. 寻找 Vulkan SDK
find_package(Vulkan REQUIRED)

# 3. 引入 vk-bootstrap
add_subdirectory(third_party/vk-bootstrap)

# 4. 设置 SDL2 路径
set(SDL2_PATH "${CMAKE_SOURCE_DIR}/third_party/SDL2")
include_directories(${SDL2_PATH}/include)
link_directories(${SDL2_PATH}/lib/x64)

# 5. 扫描源代码
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

# 6. 生成可执行文件
add_executable(VulkanEngine ${SOURCES})

# 7. 链接所有库
target_link_libraries(VulkanEngine
    PRIVATE 
    Vulkan::Vulkan
    vk-bootstrap
    SDL2
    SDL2main
)

# 告诉项目所有文件开启 GLM 实验功能 (解决 GLM 报错)
target_compile_definitions(VulkanEngine PRIVATE GLM_ENABLE_EXPERIMENTAL)

# 8. 包含头文件路径
target_include_directories(VulkanEngine PRIVATE 
    "${CMAKE_SOURCE_DIR}/third_party/glm"
    "${CMAKE_SOURCE_DIR}/third_party/vma"
)

# 9. (Windows特供) 自动拷贝 SDL2.dll
if(WIN32)
    add_custom_command(TARGET VulkanEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_PATH}/lib/x64/SDL2.dll"
        $<TARGET_FILE_DIR:VulkanEngine>)
endif()