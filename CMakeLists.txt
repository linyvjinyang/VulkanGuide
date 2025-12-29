cmake_minimum_required(VERSION 3.15)
project(VulkanGuide)

# 1. 核心标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/utf-8)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

# =========================================================
#  第三方库配置
# =========================================================

# --- 1. vk-bootstrap ---
add_subdirectory(third_party/vk-bootstrap)

# --- 2. SDL2 (使用 find_package 自动查找) ---

set(SDL2_PATH "${CMAKE_SOURCE_DIR}/third_party/SDL2")
include_directories(${SDL2_PATH}/include)
link_directories(${SDL2_PATH}/lib/x64)


# --- 3. GLM (纯头文件 - 手动指定路径) ---
set(GLM_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/glm")

# --- 4. VMA (纯头文件) ---
set(VMA_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/vma")

# =========================================================
#  主程序构建
# =========================================================

find_package(Vulkan REQUIRED)

# 自动搜集所有源文件 (不建议包含头文件)
file(GLOB SOURCES "src/*.cpp")

add_executable(VulkanEngine ${SOURCES})

target_compile_definitions(VulkanEngine PUBLIC GLM_ENABLE_EXPERIMENTAL)

# 包含路径
target_include_directories(VulkanEngine PRIVATE 
    "${CMAKE_SOURCE_DIR}/src"
    ${GLM_INCLUDE_DIR}
    ${VMA_INCLUDE_DIR}
    # SDL2 和 vk-bootstrap 的头文件会通过 target_link_libraries 自动包含
)

# 链接库

target_link_libraries(VulkanEngine
    PRIVATE 
    Vulkan::Vulkan
    vk-bootstrap
    SDL2
    SDL2main
)

# Windows 下自动把 SDL2.dll 复制到 exe 旁边
if(WIN32)
    add_custom_command(TARGET VulkanEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_PATH}/lib/x64/SDL2.dll"
        $<TARGET_FILE_DIR:VulkanEngine>)
endif()
