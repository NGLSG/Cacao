cmake_minimum_required(VERSION 4.0)
project(Cacao)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(cmake/CPM.cmake)

CPMAddPackage(
        NAME slang
        GITHUB_REPOSITORY shader-slang/slang
        GIT_TAG master
)

find_package(Vulkan REQUIRED)
file(GLOB_RECURSE SOURCE_FILES
        CONFIGURE_DEPENDS
        "src/*.cpp"
)
file(GLOB_RECURSE HEADER_FILES
        CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)


add_library(Cacao SHARED ${SOURCE_FILES} ${HEADER_FILES})
target_precompile_headers(Cacao PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/Core.h")
target_compile_definitions(Cacao PRIVATE CACAO_BUILD_DLL)

target_include_directories(Cacao PUBLIC include)

target_link_libraries(Cacao PUBLIC Vulkan::Vulkan slang)

add_executable(Test main.cpp VkContext.cpp SynchronizationContext.cpp)

target_link_libraries(Test PRIVATE Cacao)