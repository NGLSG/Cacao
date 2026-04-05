#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <cstring>
#include "Cacao.hpp"

using namespace Cacao;

struct TestResult
{
    std::string name;
    bool passed;
    std::string error;
};

static std::vector<TestResult> g_results;

#define TEST_ASSERT(cond, msg)                                          \
    do {                                                                \
        if (!(cond)) {                                                  \
            g_results.push_back({testName, false, msg});                \
            std::cerr << "  FAIL: " << msg << std::endl;               \
            return;                                                     \
        }                                                               \
    } while(0)

#define TEST_PASS()                                                     \
    g_results.push_back({testName, true, ""});                          \
    std::cout << "  PASS" << std::endl

static void TestInstanceCreation(BackendType backend)
{
    std::string testName = "Instance Creation";
    std::cout << "[" << testName << "]" << std::endl;

    InstanceCreateInfo info;
    info.type = backend;
    info.applicationName = "ConformanceTest";
    info.appVersion = 1;

    try
    {
        auto instance = Instance::Create(info);
        TEST_ASSERT(instance != nullptr, "Instance is null");
        TEST_ASSERT(instance->GetType() == backend, "Backend type mismatch");
        TEST_PASS();
    }
    catch (const std::exception& e)
    {
        g_results.push_back({testName, false, e.what()});
        std::cerr << "  FAIL: " << e.what() << std::endl;
    }
}

static void TestAdapterEnumeration(Ref<Instance>& instance)
{
    std::string testName = "Adapter Enumeration";
    std::cout << "[" << testName << "]" << std::endl;

    auto adapters = instance->EnumerateAdapters();
    TEST_ASSERT(!adapters.empty(), "No adapters found");

    auto props = adapters[0]->GetProperties();
    std::cout << "  GPU: " << props.name << std::endl;
    TEST_PASS();
}

static void TestDeviceCreation(Ref<Adapter>& adapter, Ref<Device>& outDevice)
{
    std::string testName = "Device Creation";
    std::cout << "[" << testName << "]" << std::endl;

    DeviceCreateInfo info;
    info.QueueRequests = {{QueueType::Graphics, 1, 1.0f}};

    try
    {
        outDevice = adapter->CreateDevice(info);
        TEST_ASSERT(outDevice != nullptr, "Device is null");
        TEST_ASSERT(outDevice->GetParentAdapter() == adapter, "Parent adapter mismatch");
        TEST_PASS();
    }
    catch (const std::exception& e)
    {
        g_results.push_back({testName, false, e.what()});
        std::cerr << "  FAIL: " << e.what() << std::endl;
    }
}

static void TestBufferCreation(Ref<Device>& device)
{
    std::string testName = "Buffer Create + Map/Unmap";
    std::cout << "[" << testName << "]" << std::endl;

    auto buffer = device->CreateBuffer(
        BufferBuilder()
        .SetSize(1024)
        .SetUsage(BufferUsageFlags::StorageBuffer)
        .SetMemoryUsage(BufferMemoryUsage::CpuToGpu)
        .SetName("TestBuffer")
        .Build());

    TEST_ASSERT(buffer != nullptr, "Buffer is null");
    TEST_ASSERT(buffer->GetSize() == 1024, "Buffer size mismatch");

    void* mapped = buffer->Map();
    TEST_ASSERT(mapped != nullptr, "Map returned null");

    std::memset(mapped, 0xAB, 256);
    buffer->Flush(0, 256);
    buffer->Unmap();

    TEST_PASS();
}

static void TestTextureCreation(Ref<Device>& device)
{
    std::string testName = "Texture Creation";
    std::cout << "[" << testName << "]" << std::endl;

    auto texture = device->CreateTexture(
        TextureBuilder()
        .SetType(TextureType::Texture2D)
        .SetSize(256, 256)
        .SetMipLevels(1)
        .SetArrayLayers(1)
        .SetFormat(Format::RGBA8_UNORM)
        .SetUsage(TextureUsageFlags::Sampled | TextureUsageFlags::TransferDst)
        .SetName("TestTexture")
        .Build());

    TEST_ASSERT(texture != nullptr, "Texture is null");
    TEST_ASSERT(texture->GetWidth() == 256, "Width mismatch");
    TEST_ASSERT(texture->GetHeight() == 256, "Height mismatch");
    TEST_ASSERT(texture->GetFormat() == Format::RGBA8_UNORM, "Format mismatch");

    TEST_PASS();
}

static void TestShaderCompilation(Ref<Instance>& instance, Ref<Device>& device)
{
    std::string testName = "Shader Compilation";
    std::cout << "[" << testName << "]" << std::endl;

    auto compiler = instance->CreateShaderCompiler();
    if (!compiler)
    {
        g_results.push_back({testName, false, "Shader compiler not available"});
        std::cerr << "  SKIP: Shader compiler not available" << std::endl;
        return;
    }

    ShaderCreateInfo shaderInfo;
    shaderInfo.Stage = ShaderStage::Vertex;
    shaderInfo.SourcePath = "sprite.slang";
    shaderInfo.EntryPoint = "mainVS";

    try
    {
        auto shader = compiler->CompileOrLoad(device, shaderInfo);
        TEST_ASSERT(shader != nullptr, "Vertex shader is null");
        TEST_PASS();
    }
    catch (const std::exception& e)
    {
        g_results.push_back({testName, false, e.what()});
        std::cerr << "  FAIL: " << e.what() << std::endl;
    }
}

static void TestPipelineCreation(Ref<Device>& device)
{
    std::string testName = "Pipeline Creation";
    std::cout << "[" << testName << "]" << std::endl;

    auto layout = device->CreateDescriptorSetLayout(
        DescriptorSetLayoutBuilder()
        .AddBinding(0, DescriptorType::StorageBuffer, 1, ShaderStage::Vertex)
        .Build());

    TEST_ASSERT(layout != nullptr, "DescriptorSetLayout is null");

    auto pipelineLayout = device->CreatePipelineLayout(
        PipelineLayoutBuilder()
        .AddSetLayout(layout)
        .Build());

    if (!pipelineLayout)
    {
        g_results.push_back({testName, false, "PipelineLayout not supported on this backend"});
        std::cerr << "  SKIP: PipelineLayout not available" << std::endl;
        return;
    }

    TEST_PASS();
}

static void RunBackendTests(BackendType backend, const std::string& name)
{
    std::cout << "\n====== Testing Backend: " << name << " ======" << std::endl;

    InstanceCreateInfo info;
    info.type = backend;
    info.applicationName = "ConformanceTest";
    info.appVersion = 1;

    Ref<Instance> instance;
    try
    {
        instance = Instance::Create(info);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Cannot create instance for " << name << ": " << e.what() << std::endl;
        g_results.push_back({"[" + name + "] Instance", false, e.what()});
        return;
    }

    TestInstanceCreation(backend);

    auto adapters = instance->EnumerateAdapters();
    if (adapters.empty())
    {
        std::cerr << "No adapters for " << name << ", skipping device tests" << std::endl;
        return;
    }

    TestAdapterEnumeration(instance);

    auto adapter = adapters[0];
    Ref<Device> device;
    TestDeviceCreation(adapter, device);
    if (!device) return;

    TestBufferCreation(device);
    TestTextureCreation(device);
    TestShaderCompilation(instance, device);
    TestPipelineCreation(device);
}

int main()
{
    std::cout << "=== Cacao RHI Conformance Test ===" << std::endl;

    struct BackendEntry { BackendType type; std::string name; };
    std::vector<BackendEntry> backends;

    backends.push_back({BackendType::Vulkan, "Vulkan"});


    if (backends.empty())
    {
        std::cerr << "No backends compiled! Enable at least one CACAO_BACKEND_* option." << std::endl;
        return 1;
    }

    for (auto& be : backends)
    {
        try
        {
            RunBackendTests(be.type, be.name);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Unhandled exception in " << be.name << ": " << e.what() << std::endl;
        }
    }

    // Summary
    std::cout << "\n====== Results ======" << std::endl;
    int passed = 0, failed = 0;
    for (auto& r : g_results)
    {
        if (r.passed)
        {
            passed++;
            std::cout << "  PASS: " << r.name << std::endl;
        }
        else
        {
            failed++;
            std::cout << "  FAIL: " << r.name << " - " << r.error << std::endl;
        }
    }
    std::cout << "\nTotal: " << (passed + failed) << " | Passed: " << passed << " | Failed: " << failed << std::endl;

    return failed > 0 ? 1 : 0;
}
