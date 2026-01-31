#ifdef WIN32
#include "Impls/D3D12/D3D12Device.h"
#include "Impls/D3D12/D3D12Adapter.h"
#include "Impls/D3D12/D3D12Queue.h"
#include "Impls/D3D12/D3D12Swapchain.h"
#include "Impls/D3D12/D3D12Buffer.h"
#include "Impls/D3D12/D3D12Texture.h"
#include "Impls/D3D12/D3D12Sampler.h"
#include "Impls/D3D12/D3D12DescriptorPool.h"
#include "Impls/D3D12/D3D12DescriptorSetLayout.h"
#include "Impls/D3D12/D3D12ShaderModule.h"
#include "Impls/D3D12/D3D12PipelineLayout.h"
#include "Impls/D3D12/D3D12Pipeline.h"
#include "Impls/D3D12/D3D12Synchronization.h"
#include "Impls/D3D12/D3D12CommandBufferEncoder.h"
#include <iostream>
#include <sstream>

namespace Cacao
{
    Ref<D3D12Device> D3D12Device::Create(const Ref<Adapter>& adapter, const DeviceCreateInfo& createInfo)
    {
        return CreateRef<D3D12Device>(adapter, createInfo);
    }

    D3D12Device::D3D12Device(const Ref<Adapter>& adapter, const DeviceCreateInfo& createInfo)
{
    if (! adapter)
    {
        throw std::runtime_error("D3D12Device created with null adapter");
    }

    m_createInfo = createInfo;
    m_parentAdapter = std::dynamic_pointer_cast<D3D12Adapter>(adapter);
    if (!m_parentAdapter)
    {
        throw std::runtime_error("D3D12Device requires a D3D12Adapter");
    }

    // 创建 D3D12 设备
    HRESULT hr = D3D12CreateDevice(
        m_parentAdapter->GetDXGIAdapter(). Get(),
        D3D_FEATURE_LEVEL_12_1,
        IID_PPV_ARGS(&m_d3dDevice)
    );

    if (FAILED(hr))
    {
        std::ostringstream oss;
        oss << "Failed to create D3D12 device. HRESULT: 0x"
            << std::hex << static_cast<unsigned int>(hr);
        throw std::runtime_error(oss.str());
    }

    // 检查点 1: 设备创建后立即检查状态
    HRESULT deviceStatus = m_d3dDevice->GetDeviceRemovedReason();
    if (deviceStatus != S_OK)
    {
        std::ostringstream oss;
        oss << "[Checkpoint 1] Device removed immediately after creation.  Reason: 0x"
            << std::hex << static_cast<unsigned int>(deviceStatus);
        throw std::runtime_error(oss. str());
    }
    std::cout << "[D3D12] Device created successfully" << std::endl;

    // 创建 D3D12MA 分配器
    D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
    allocatorDesc. pDevice = m_d3dDevice. Get();
    allocatorDesc.pAdapter = m_parentAdapter->GetDXGIAdapter().Get();
    allocatorDesc.Flags = D3D12MA::ALLOCATOR_FLAG_NONE;

    hr = D3D12MA::CreateAllocator(&allocatorDesc, &m_allocator);
    if (FAILED(hr) || !m_allocator)
    {
        std::ostringstream oss;
        oss << "Failed to create D3D12MA allocator. HRESULT: 0x"
            << std::hex << static_cast<unsigned int>(hr);
        throw std::runtime_error(oss.str());
    }

    // 检查点 2: 分配器创建后检查状态
    deviceStatus = m_d3dDevice->GetDeviceRemovedReason();
    if (deviceStatus != S_OK)
    {
        std::ostringstream oss;
        oss << "[Checkpoint 2] Device removed after allocator creation.  Reason: 0x"
            << std::hex << static_cast<unsigned int>(deviceStatus);
        throw std::runtime_error(oss. str());
    }
    std::cout << "[D3D12] D3D12MA allocator created successfully" << std::endl;

    // 创建命令队列（如果你有的话）
    // ...

    // 检查点 3: 检查所有初始化完成后的状态
    deviceStatus = m_d3dDevice->GetDeviceRemovedReason();
    if (deviceStatus != S_OK)
    {
        std::ostringstream oss;
        oss << "[Checkpoint 3] Device removed after initialization. Reason: 0x"
            << std::hex << static_cast<unsigned int>(deviceStatus);
        throw std::runtime_error(oss.str());
    }

    // 根据请求的特性检查支持情况
    for (const auto& feature : createInfo.EnabledFeatures)
    {
        D3D12_FEATURE d3dFeature = D3D12Converter::Convert(feature);

        switch (feature)
        {
        case DeviceFeature::MeshShader:
            {
                D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
                if (SUCCEEDED(
                    m_d3dDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7))))
                {
                    if (options7. MeshShaderTier == D3D12_MESH_SHADER_TIER_NOT_SUPPORTED)
                    {
                        std::cerr << "Warning: Mesh shaders not supported on this device" << std::endl;
                    }
                }
                break;
            }
        case DeviceFeature::RayTracingPipeline:
        case DeviceFeature::AccelerationStructure:
            {
                D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
                if (SUCCEEDED(
                    m_d3dDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5))))
                {
                    if (options5. RaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
                    {
                        std::cerr << "Warning: Ray tracing not supported on this device" << std::endl;
                    }
                }
                break;
            }
        case DeviceFeature::VariableRateShading:
            {
                D3D12_FEATURE_DATA_D3D12_OPTIONS6 options6 = {};
                if (SUCCEEDED(
                    m_d3dDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &options6, sizeof(options6))))
                {
                    if (options6. VariableShadingRateTier == D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED)
                    {
                        std::cerr << "Warning: Variable rate shading not supported on this device" << std::endl;
                    }
                }
                break;
            }
        default:
            break;
        }
    }

    // 最终检查点
    deviceStatus = m_d3dDevice->GetDeviceRemovedReason();
    if (deviceStatus != S_OK)
    {
        std::ostringstream oss;
        oss << "[Final Checkpoint] Device removed after feature checks. Reason: 0x"
            << std::hex << static_cast<unsigned int>(deviceStatus);
        throw std::runtime_error(oss.str());
    }

    std::cout << "[D3D12] Device initialization complete" << std::endl;
}

    D3D12Device::~D3D12Device()
    {
        // 等待所有队列完成
        if (m_graphicsQueue)
        {
            m_graphicsQueue->WaitIdle();
        }
        if (m_computeQueue)
        {
            m_computeQueue->WaitIdle();
        }
        if (m_copyQueue)
        {
            m_copyQueue->WaitIdle();
        }

        // 清理命令分配器
        {
            std::lock_guard<std::mutex> lock(m_commandAllocatorMutex);
            for (auto& [threadId, allocatorData] : m_threadCommandAllocators)
            {
                while (!allocatorData.primaryBuffers.empty())
                {
                    allocatorData.primaryBuffers.pop();
                }
                while (!allocatorData.secondaryBuffers.empty())
                {
                    allocatorData.secondaryBuffers.pop();
                }
            }
            m_threadCommandAllocators.clear();
        }

        // 销毁 D3D12MA 分配器
        if (m_allocator)
        {
            m_allocator->Release();
            m_allocator = nullptr;
        }

        // 清理队列引用
        m_graphicsQueue.reset();
        m_computeQueue.reset();
        m_copyQueue.reset();

        // ComPtr 会自动释放设备
    }

    D3D12ThreadCommandAllocatorData& D3D12Device::GetThreadCommandAllocator()
    {
        std::thread::id thisThread = std::this_thread::get_id();

        {
            std::lock_guard<std::mutex> lock(m_commandAllocatorMutex);
            auto it = m_threadCommandAllocators.find(thisThread);
            if (it != m_threadCommandAllocators.end())
            {
                return it->second;
            }
        }

        // 创建新的命令分配器
        D3D12ThreadCommandAllocatorData newAllocatorData;

        // Direct 命令分配器（用于图形和通用命令）
        HRESULT hr = m_d3dDevice->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&newAllocatorData.directAllocator)
        );
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create direct command allocator");
        }

        // Compute 命令分配器
        hr = m_d3dDevice->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_COMPUTE,
            IID_PPV_ARGS(&newAllocatorData. computeAllocator)
        );
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create compute command allocator");
        }

        // Copy 命令分配器
        hr = m_d3dDevice->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_COPY,
            IID_PPV_ARGS(&newAllocatorData.copyAllocator)
        );
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create copy command allocator");
        }

        {
            std::lock_guard<std::mutex> lock(m_commandAllocatorMutex);
            auto [it, inserted] = m_threadCommandAllocators.emplace(thisThread, std::move(newAllocatorData));
            return it->second;
        }
    }

    Ref<Queue> D3D12Device::GetQueue(QueueType type, uint32_t index)
    {
        // D3D12 中队列是按需创建的，这里我们缓存常用的队列
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.NodeMask = 0;

        switch (type)
        {
        case QueueType::Present:
        case QueueType::Graphics:
            {
                if (m_graphicsQueue && index == 0)
                {
                    return m_graphicsQueue;
                }
                queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
                break;
            }
        case QueueType::Compute:
            {
                if (m_computeQueue && index == 0)
                {
                    return m_computeQueue;
                }
                queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
                break;
            }
        case QueueType::Transfer:
            {
                if (m_copyQueue && index == 0)
                {
                    return m_copyQueue;
                }
                queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
                break;
            }
        default:
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            break;
        }

        ComPtr<ID3D12CommandQueue> d3dQueue;
        HRESULT hr = m_d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&d3dQueue));
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create D3D12 command queue");
        }

        auto queue = D3D12Queue::Create(shared_from_this(), d3dQueue, type, index);

        // 缓存索引为 0 的队列
        if (index == 0)
        {
            switch (type)
            {
            case QueueType::Present:
            case QueueType::Graphics:
                m_graphicsQueue = queue;
                break;
            case QueueType::Compute:
                m_computeQueue = queue;
                break;
            case QueueType::Transfer:
                m_copyQueue = queue;
                break;
            default:
                break;
            }
        }

        return queue;
    }

    Ref<Swapchain> D3D12Device::CreateSwapchain(const SwapchainCreateInfo& createInfo)
    {
        return D3D12Swapchain::Create(shared_from_this(), createInfo);
    }

    std::vector<uint32_t> D3D12Device::GetAllQueueFamilyIndices() const
    {
        // D3D12 没有队列族概念，返回单一索引
        return {0};
    }

    Ref<Adapter> D3D12Device::GetParentAdapter() const
    {
        return m_parentAdapter;
    }

    Ref<CommandBufferEncoder> D3D12Device::CreateCommandBufferEncoder(CommandBufferType type)
    {
        auto& allocatorData = GetThreadCommandAllocator();

        // 检查是否有可复用的命令缓冲区
        if (type == CommandBufferType::Primary && !allocatorData.primaryBuffers.empty())
        {
            auto encoder = allocatorData.primaryBuffers.front();
            allocatorData.primaryBuffers.pop();
            return encoder;
        }
        else if (type == CommandBufferType::Secondary && !allocatorData.secondaryBuffers.empty())
        {
            auto encoder = allocatorData.secondaryBuffers.front();
            allocatorData.secondaryBuffers.pop();
            return encoder;
        }

        // 创建新的命令列表
        // D3D12 中命令列表类型对应不同的功能
        // Primary -> Direct (可以执行所有命令)
        // Secondary -> Bundle (预录制的命令序列)
        D3D12_COMMAND_LIST_TYPE listType = (type == CommandBufferType::Primary)
                                               ? D3D12_COMMAND_LIST_TYPE_DIRECT
                                               : D3D12_COMMAND_LIST_TYPE_BUNDLE;

        ID3D12CommandAllocator* allocator = (type == CommandBufferType::Primary)
                                                ? allocatorData.directAllocator.Get()
                                                : allocatorData.directAllocator.Get(); // Bundle 也使用 direct allocator

        ComPtr<ID3D12GraphicsCommandList> commandList;
        HRESULT hr = m_d3dDevice->CreateCommandList(
            0,
            listType,
            allocator,
            nullptr, // 初始 PSO
            IID_PPV_ARGS(&commandList)
        );

        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create D3D12 command list");
        }

        // 命令列表创建后处于打开状态，需要关闭
        commandList->Close();

        return D3D12CommandBufferEncoder::Create(shared_from_this(), commandList, allocator, type);
    }

    void D3D12Device::ResetCommandPool()
    {
        auto& allocatorData = GetThreadCommandAllocator();

        // 重置所有命令分配器
        if (allocatorData.directAllocator)
        {
            allocatorData.directAllocator->Reset();
        }
        if (allocatorData.computeAllocator)
        {
            allocatorData.computeAllocator->Reset();
        }
        if (allocatorData.copyAllocator)
        {
            allocatorData.copyAllocator->Reset();
        }
    }

    void D3D12Device::ReturnCommandBuffer(const Ref<CommandBufferEncoder>& encoder)
    {
        auto& allocatorData = GetThreadCommandAllocator();
        auto d3d12Encoder = std::dynamic_pointer_cast<D3D12CommandBufferEncoder>(encoder);

        if (!d3d12Encoder)
        {
            throw std::runtime_error("Invalid command buffer encoder type");
        }

        if (encoder->GetCommandBufferType() == CommandBufferType::Primary)
        {
            allocatorData.primaryBuffers.push(d3d12Encoder);
        }
        else
        {
            allocatorData.secondaryBuffers.push(d3d12Encoder);
        }
    }

    void D3D12Device::FreeCommandBuffer(const Ref<CommandBufferEncoder>& encoder)
    {
        // D3D12 中命令列表会自动释放
        // 这里我们只需要确保不再持有引用
        // ComPtr 会自动处理释放
    }

    void D3D12Device::ResetCommandBuffer(const Ref<CommandBufferEncoder>& encoder)
    {
        auto d3d12Encoder = std::dynamic_pointer_cast<D3D12CommandBufferEncoder>(encoder);
        if (!d3d12Encoder)
        {
            throw std::runtime_error("Invalid command buffer encoder type");
        }

        if (encoder->GetCommandBufferType() == CommandBufferType::Primary)
        // 重置命令列表
            d3d12Encoder->GetCommandList()->Reset(
                (encoder->GetCommandBufferType() == CommandBufferType::Primary)
                    ? GetThreadCommandAllocator().directAllocator.Get()
                    : GetThreadCommandAllocator().directAllocator.Get(), // Bundle 也使用 direct allocator
                nullptr
            );
        else
        {
            d3d12Encoder->GetCommandList()->Reset(
                GetThreadCommandAllocator().directAllocator.Get(),
                nullptr
            );
        }
    }

    Ref<Texture> D3D12Device::CreateTexture(const TextureCreateInfo& createInfo)
    {
        return D3D12Texture::Create(shared_from_this(), m_allocator, createInfo);
    }

    Ref<Buffer> D3D12Device::CreateBuffer(const BufferCreateInfo& createInfo)
    {
        return D3D12Buffer::Create(shared_from_this(), m_allocator, createInfo);
    }

    Ref<Sampler> D3D12Device::CreateSampler(const SamplerCreateInfo& createInfo)
    {
        return D3D12Sampler::Create(shared_from_this(), createInfo);
    }

    Ref<DescriptorSetLayout> D3D12Device::CreateDescriptorSetLayout(const DescriptorSetLayoutCreateInfo& info)
    {
        return D3D12DescriptorSetLayout::Create(shared_from_this(), info);
    }

    Ref<DescriptorPool> D3D12Device::CreateDescriptorPool(const DescriptorPoolCreateInfo& info)
    {
        return D3D12DescriptorPool::Create(shared_from_this(), info);
    }

    Ref<ShaderModule> D3D12Device::CreateShaderModule(const ShaderBlob& blob, const ShaderCreateInfo& info)
    {
        return D3D12ShaderModule::Create(shared_from_this(), info, blob);
    }

    Ref<PipelineLayout> D3D12Device::CreatePipelineLayout(const PipelineLayoutCreateInfo& info)
    {
        return D3D12PipelineLayout::Create(shared_from_this(), info);
    }

    Ref<PipelineCache> D3D12Device::CreatePipelineCache(std::span<const uint8_t> initialData)
    {
        return D3D12PipelineCache::Create(shared_from_this(), initialData);
    }

    Ref<GraphicsPipeline> D3D12Device::CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& info)
    {
        return D3D12GraphicsPipeline::Create(shared_from_this(), info);
    }

    Ref<ComputePipeline> D3D12Device::CreateComputePipeline(const ComputePipelineCreateInfo& info)
    {
        return D3D12ComputePipeline::Create(shared_from_this(), info);
    }

    Ref<Synchronization> D3D12Device::CreateSynchronization(uint32_t maxFramesInFlight)
    {
        return D3D12Synchronization::Create(shared_from_this(), maxFramesInFlight);
    }
}
#endif
