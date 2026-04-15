cmake_minimum_required(VERSION 4.0)
project(Cacao)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(cmake/CPM.cmake)

CPMAddPackage(
        NAME slang
        GITHUB_REPOSITORY shader-slang/slang
        GIT_TAG v2026.5.2
)

# Suppress Slang bootstrap memory leak assertions in Debug builds
if(TARGET slang-bootstrap)
    target_compile_definitions(slang-bootstrap PRIVATE NDEBUG)
endif()
if(TARGET slangc)
    target_compile_definitions(slangc PRIVATE NDEBUG)
endif()

# ==================== Backend Options ====================
option(CACAO_BACKEND_VULKAN   "Enable Vulkan backend"     ON)
option(CACAO_BACKEND_OPENGL   "Enable OpenGL 4.5 backend" OFF)
option(CACAO_BACKEND_OPENGLES "Enable OpenGL ES 3.2 backend" OFF)
if(WIN32)
    option(CACAO_BACKEND_D3D12 "Enable Direct3D 12 backend" ON)
    option(CACAO_BACKEND_D3D11 "Enable Direct3D 11 backend" ON)
else()
    option(CACAO_BACKEND_D3D12 "Enable Direct3D 12 backend" OFF)
    option(CACAO_BACKEND_D3D11 "Enable Direct3D 11 backend" OFF)
endif()

# ==================== Core Sources ====================
file(GLOB CORE_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB CORE_HEADERS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)

set(ALL_SOURCES ${CORE_SOURCES})
set(ALL_HEADERS ${CORE_HEADERS})

# ==================== Backend Sources ====================
if(CACAO_BACKEND_VULKAN)
    find_package(Vulkan REQUIRED)
    file(GLOB_RECURSE VK_SOURCES CONFIGURE_DEPENDS "src/Impls/Vulkan/*.cpp")
    file(GLOB_RECURSE VK_HEADERS CONFIGURE_DEPENDS "include/Impls/Vulkan/*.h")
    list(APPEND ALL_SOURCES ${VK_SOURCES})
    list(APPEND ALL_HEADERS ${VK_HEADERS})
endif()

if(CACAO_BACKEND_D3D12)
    file(GLOB_RECURSE D3D12_SOURCES CONFIGURE_DEPENDS "src/Impls/D3D12/*.cpp")
    file(GLOB_RECURSE D3D12_HEADERS CONFIGURE_DEPENDS "include/Impls/D3D12/*.h")
    list(APPEND ALL_SOURCES ${D3D12_SOURCES}
            "${CMAKE_CURRENT_SOURCE_DIR}/third_party/D3D12MA/D3D12MemAlloc.cpp")
    list(APPEND ALL_HEADERS ${D3D12_HEADERS}
            "${CMAKE_CURRENT_SOURCE_DIR}/third_party/D3D12MA/D3D12MemAlloc.h")
endif()

if(CACAO_BACKEND_D3D11)
    file(GLOB_RECURSE D3D11_SOURCES CONFIGURE_DEPENDS "src/Impls/D3D11/*.cpp")
    file(GLOB_RECURSE D3D11_HEADERS CONFIGURE_DEPENDS "include/Impls/D3D11/*.h")
    list(APPEND ALL_SOURCES ${D3D11_SOURCES})
    list(APPEND ALL_HEADERS ${D3D11_HEADERS})
endif()

if(CACAO_BACKEND_OPENGL OR CACAO_BACKEND_OPENGLES)
    file(GLOB_RECURSE GL_SOURCES CONFIGURE_DEPENDS "src/Impls/OpenGL/*.cpp")
    file(GLOB_RECURSE GL_HEADERS CONFIGURE_DEPENDS "include/Impls/OpenGL/*.h")
    list(APPEND ALL_SOURCES ${GL_SOURCES})
    list(APPEND ALL_HEADERS ${GL_HEADERS})
endif()

# ==================== Library Target ====================
add_library(Cacao SHARED ${ALL_SOURCES} ${ALL_HEADERS})
target_precompile_headers(Cacao PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/Core.h")
target_compile_definitions(Cacao PRIVATE CACAO_BUILD_DLL)
target_include_directories(Cacao PUBLIC include)

# ==================== Backend Definitions & Links ====================
if(CACAO_BACKEND_VULKAN)
    target_compile_definitions(Cacao PUBLIC CACAO_BACKEND_VULKAN=1)
    target_link_libraries(Cacao PUBLIC Vulkan::Vulkan)
endif()

if(CACAO_BACKEND_D3D12)
    target_compile_definitions(Cacao PUBLIC CACAO_BACKEND_D3D12=1)
    target_include_directories(Cacao PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/D3D12MA")
    target_link_libraries(Cacao PUBLIC d3d12 dxgi)
endif()

if(CACAO_BACKEND_D3D11)
    target_compile_definitions(Cacao PUBLIC CACAO_BACKEND_D3D11=1)
    target_link_libraries(Cacao PUBLIC d3d11)
endif()

if(CACAO_BACKEND_OPENGL)
    add_library(glad STATIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad/src/glad.c)
    target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad/include)
    target_compile_definitions(Cacao PUBLIC CACAO_BACKEND_OPENGL=1 CACAO_GL=1)
    target_link_libraries(Cacao PUBLIC glad)
    if(WIN32)
        target_link_libraries(Cacao PUBLIC opengl32)
    else()
        find_package(OpenGL REQUIRED)
        target_link_libraries(Cacao PUBLIC OpenGL::GL)
    endif()
endif()

if(CACAO_BACKEND_OPENGLES)
    target_compile_definitions(Cacao PUBLIC CACAO_BACKEND_OPENGLES=1 CACAO_GLES=1)
    target_link_libraries(Cacao PUBLIC GLESv2 EGL)
endif()

target_link_libraries(Cacao PUBLIC slang)

# ==================== Test Executable ====================
add_executable(Test main.cpp)
target_link_libraries(Test PRIVATE Cacao)

# ==================== Hello Triangle Demo ====================
add_executable(HelloTriangle demos/hello_triangle.cpp)
# target_compile_definitions(HelloTriangle PRIVATE CACAO_USE_VULKAN)
target_link_libraries(HelloTriangle PRIVATE Cacao)
target_include_directories(HelloTriangle PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/D3D12MA")
add_custom_command(TARGET HelloTriangle POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/demos/hello_triangle.slang"
    "$<TARGET_FILE_DIR:HelloTriangle>/hello_triangle.slang")

# ==================== 3D Scene Demo ====================
add_executable(Scene3D demos/scene_3d.cpp)
target_link_libraries(Scene3D PRIVATE Cacao)
target_include_directories(Scene3D PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/D3D12MA")
add_custom_command(TARGET Scene3D POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/demos/scene_3d.slang"
    "$<TARGET_FILE_DIR:Scene3D>/scene_3d.slang")

# ==================== Textured Quad Demo ====================
add_executable(TexturedQuad demos/textured_quad.cpp)
target_link_libraries(TexturedQuad PRIVATE Cacao)
target_include_directories(TexturedQuad PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/D3D12MA")
add_custom_command(TARGET TexturedQuad POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/demos/textured_quad.slang"
    "$<TARGET_FILE_DIR:TexturedQuad>/textured_quad.slang")

# ==================== Cornell Box Ray Tracing Demo ====================
add_executable(CornellBox demos/cornell_box.cpp)
target_compile_definitions(CornellBox PRIVATE CACAO_USE_VULKAN)
target_link_libraries(CornellBox PRIVATE Cacao)
target_include_directories(CornellBox PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/D3D12MA")
add_custom_command(TARGET CornellBox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/demos/cornell_box.slang"
    "$<TARGET_FILE_DIR:CornellBox>/cornell_box.slang")

# ==================== Compute Demo ====================
add_executable(ComputeDemo demos/compute_demo.cpp)
# target_compile_definitions(ComputeDemo PRIVATE CACAO_USE_VULKAN)
target_link_libraries(ComputeDemo PRIVATE Cacao)
target_include_directories(ComputeDemo PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/D3D12MA")
add_custom_command(TARGET ComputeDemo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/demos/compute_demo.slang"
    "$<TARGET_FILE_DIR:ComputeDemo>/compute_demo.slang")


# ==================== Shadow Map Demo ====================
add_executable(ShadowMap demos/shadow_map.cpp)
target_compile_definitions(ShadowMap PRIVATE CACAO_USE_VULKAN)
target_link_libraries(ShadowMap PRIVATE Cacao)
target_include_directories(ShadowMap PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/D3D12MA")
add_custom_command(TARGET ShadowMap POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/demos/shadow_map.slang"
    "$<TARGET_FILE_DIR:ShadowMap>/shadow_map.slang")

# ==================== DX12 Raw Benchmark ====================
if(WIN32 AND CACAO_BACKEND_D3D12)
    add_executable(DX12Benchmark DX12Benchmark.cpp)
    target_link_libraries(DX12Benchmark PRIVATE d3d12 dxgi d3dcompiler)
endif()

# ==================== CTest Conformance Tests ====================
enable_testing()
add_executable(ConformanceTest tests/conformance_test.cpp)
target_link_libraries(ConformanceTest PRIVATE Cacao)
add_custom_command(TARGET ConformanceTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/sprite.slang"
    "$<TARGET_FILE_DIR:ConformanceTest>/sprite.slang")
add_test(NAME ConformanceTest COMMAND ConformanceTest)
