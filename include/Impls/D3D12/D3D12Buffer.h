#ifndef CACAO_D3D12BUFFER_H
#define CACAO_D3D12BUFFER_H
#ifdef WIN32
#include "Buffer.h"
#include "D3D12Common.h"
namespace Cacao
{
    class CACAO_API D3D12Buffer : public Buffer
    {
        ComPtr<IDXGIResource1> m_dxgiResource;
    public:
        uint64_t GetSize() const override;
        BufferUsageFlags GetUsage() const override;
        BufferMemoryUsage GetMemoryUsage() const override;
        void* Map() override;
        void Unmap() override;
        void Flush(uint64_t offset, uint64_t size) override;
        uint64_t GetDeviceAddress() const override;
    };
} 
#endif
#endif 
