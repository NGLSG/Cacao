#ifndef CACAO_D3D12DEVICE_H
#define CACAO_D3D12DEVICE_H
#ifdef WIN32
#include "Device.h"
#include "D3D12Common.h"
#include "D3D12MemAlloc.h"
#include <queue>
#include <mutex>
#include <unordered_map>
#include <thread>

namespace Cacao
{
    class D3D12Adapter;
    class D3D12CommandBufferEncoder;

    // 每个线程的命令分配器数据
    struct D3D12ThreadCommandAllocatorData
    {
        ComPtr<ID3D12CommandAllocator> directAllocator;
        ComPtr<ID3D12CommandAllocator> computeAllocator;
        ComPtr<ID3D12CommandAllocator> copyAllocator;
        std::queue<Ref<D3D12CommandBufferEncoder>> primaryBuffers;
        std::queue<Ref<D3D12CommandBufferEncoder>> secondaryBuffers;
    };

    class CACAO_API D3D12Device : public Device
    {
        ComPtr<ID3D12Device14> m_d3dDevice;
        Ref<D3D12Adapter> m_parentAdapter;
        DeviceCreateInfo m_createInfo;

        // D3D12MA 分配器
        D3D12MA::Allocator* m_allocator = nullptr;

        // 命令分配器管理
        std::mutex m_commandAllocatorMutex;
        std::unordered_map<std::thread::id, D3D12ThreadCommandAllocatorData> m_threadCommandAllocators;

        // 缓存的队列
        Ref<Queue> m_graphicsQueue;
        Ref<Queue> m_computeQueue;
        Ref<Queue> m_copyQueue;

        friend class D3D12Synchronization;
        friend class D3D12Swapchain;
        friend class D3D12Queue;
        friend class D3D12Buffer;
        friend class D3D12Texture;
        friend class D3D12TextureView;
        friend class D3D12Sampler;
        friend class D3D12DescriptorPool;
        friend class D3D12DescriptorSetLayout;
        friend class D3D12DescriptorSet;
        friend class D3D12ShaderModule;
        friend class D3D12PipelineLayout;
        friend class D3D12PipelineCache;
        friend class D3D12GraphicsPipeline;
        friend class D3D12ComputePipeline;

        D3D12ThreadCommandAllocatorData& GetThreadCommandAllocator();
        D3D12MA::Allocator* GetD3D12MAAllocator() { return m_allocator; }

    public:
        static Ref<D3D12Device> Create(const Ref<Adapter>& adapter, const DeviceCreateInfo& createInfo);
        D3D12Device(const Ref<Adapter>& adapter, const DeviceCreateInfo& createInfo);
        ~D3D12Device() override;

        Ref<Queue> GetQueue(QueueType type, uint32_t index) override;
        Ref<Swapchain> CreateSwapchain(const SwapchainCreateInfo& createInfo) override;
        std::vector<uint32_t> GetAllQueueFamilyIndices() const override;
        Ref<Adapter> GetParentAdapter() const override;
        Ref<CommandBufferEncoder> CreateCommandBufferEncoder(CommandBufferType type) override;
        void ResetCommandPool() override;
        void ReturnCommandBuffer(const Ref<CommandBufferEncoder>& encoder) override;
        void FreeCommandBuffer(const Ref<CommandBufferEncoder>& encoder) override;
        void ResetCommandBuffer(const Ref<CommandBufferEncoder>& encoder) override;
        Ref<Texture> CreateTexture(const TextureCreateInfo& createInfo) override;
        Ref<Buffer> CreateBuffer(const BufferCreateInfo& createInfo) override;
        Ref<Sampler> CreateSampler(const SamplerCreateInfo& createInfo) override;
        Ref<DescriptorSetLayout> CreateDescriptorSetLayout(const DescriptorSetLayoutCreateInfo& info) override;
        Ref<DescriptorPool> CreateDescriptorPool(const DescriptorPoolCreateInfo& info) override;
        Ref<ShaderModule> CreateShaderModule(const ShaderBlob& blob, const ShaderCreateInfo& info) override;
        Ref<PipelineLayout> CreatePipelineLayout(const PipelineLayoutCreateInfo& info) override;
        Ref<PipelineCache> CreatePipelineCache(std::span<const uint8_t> initialData) override;
        Ref<GraphicsPipeline> CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& info) override;
        Ref<ComputePipeline> CreateComputePipeline(const ComputePipelineCreateInfo& info) override;
        Ref<Synchronization> CreateSynchronization(uint32_t maxFramesInFlight) override;

        ComPtr<ID3D12Device14>& GetHandle() { return m_d3dDevice; }
    };
}
#endif
#endif