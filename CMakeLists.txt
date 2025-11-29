cmake_minimum_required(VERSION 4.0)
project(Cacao)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
# 添加源文件
add_executable(Cacao main.cpp VkContext.cpp SynchronizationContext.cpp)
# 链接 Vulkan 和 Windows 系统库
target_link_libraries(Cacao
        PRIVATE
        Vulkan::Vulkan
)