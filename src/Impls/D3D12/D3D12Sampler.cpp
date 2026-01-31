#ifdef WIN32
#include "Impls/D3D12/D3D12Sampler.h"
#include "Impls/D3D12/D3D12Device.h"
#include <stdexcept>
#include <algorithm>

namespace Cacao
{
    D3D12Sampler::D3D12Sampler(const Ref<Device>& device, const SamplerCreateInfo& createInfo)
        : m_createInfo(createInfo)
    {
        if (! device)
        {
            throw std::runtime_error("D3D12Sampler created with null device");
        }

        m_device = std::dynamic_pointer_cast<D3D12Device>(device);
        if (!m_device)
        {
            throw std::runtime_error("D3D12Sampler requires a D3D12Device");
        }

        // 初始化采样器描述
        m_samplerDesc = {};

        // 设置过滤器
        m_samplerDesc. Filter = D3D12Converter::ConvertFilter(
            createInfo.MinFilter,
            createInfo.MagFilter,
            createInfo.MipmapMode,
            createInfo.AnisotropyEnable
        );

        // 如果启用了比较功能，需要修改过滤器
        if (createInfo. CompareEnable)
        {
            // 将过滤器转换为比较过滤器
            // D3D12 比较过滤器的值 = 普通过滤器值 + 0x80
            m_samplerDesc.Filter = static_cast<D3D12_FILTER>(
                static_cast<int>(m_samplerDesc.Filter) | 0x80
            );
        }

        // 设置地址模式
        m_samplerDesc. AddressU = D3D12Converter::Convert(createInfo.AddressModeU);
        m_samplerDesc.AddressV = D3D12Converter::Convert(createInfo.AddressModeV);
        m_samplerDesc.AddressW = D3D12Converter::Convert(createInfo.AddressModeW);

        // 设置 LOD 偏移
        m_samplerDesc.MipLODBias = createInfo. MipLodBias;

        // 设置各向异性过滤
        if (createInfo.AnisotropyEnable)
        {
            m_samplerDesc.MaxAnisotropy = static_cast<UINT>(std::clamp(createInfo. MaxAnisotropy, 1.0f, 16.0f));
        }
        else
        {
            m_samplerDesc.MaxAnisotropy = 1;
        }

        // 设置比较函数
        if (createInfo.CompareEnable)
        {
            m_samplerDesc.ComparisonFunc = D3D12Converter::Convert(createInfo. CompareOp);
        }
        else
        {
            m_samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        }

        // 设置边框颜色
        switch (createInfo.BorderColor)
        {
        case BorderColor::FloatTransparentBlack:
        case BorderColor::IntTransparentBlack:
            m_samplerDesc.BorderColor[0] = 0.0f;
            m_samplerDesc. BorderColor[1] = 0.0f;
            m_samplerDesc. BorderColor[2] = 0.0f;
            m_samplerDesc. BorderColor[3] = 0.0f;
            break;

        case BorderColor::FloatOpaqueBlack:
        case BorderColor::IntOpaqueBlack:
            m_samplerDesc.BorderColor[0] = 0.0f;
            m_samplerDesc.BorderColor[1] = 0.0f;
            m_samplerDesc.BorderColor[2] = 0.0f;
            m_samplerDesc.BorderColor[3] = 1.0f;
            break;

        case BorderColor::FloatOpaqueWhite:
        case BorderColor::IntOpaqueWhite:
            m_samplerDesc.BorderColor[0] = 1.0f;
            m_samplerDesc.BorderColor[1] = 1.0f;
            m_samplerDesc.BorderColor[2] = 1.0f;
            m_samplerDesc.BorderColor[3] = 1.0f;
            break;

        default:
            m_samplerDesc. BorderColor[0] = 0.0f;
            m_samplerDesc. BorderColor[1] = 0.0f;
            m_samplerDesc. BorderColor[2] = 0.0f;
            m_samplerDesc. BorderColor[3] = 1.0f;
            break;
        }

        // 设置 LOD 范围
        m_samplerDesc.MinLOD = createInfo.MinLod;
        m_samplerDesc.MaxLOD = createInfo.MaxLod;
    }

    Ref<D3D12Sampler> D3D12Sampler::Create(const Ref<Device>& device, const SamplerCreateInfo& createInfo)
    {
        return CreateRef<D3D12Sampler>(device, createInfo);
    }

    const SamplerCreateInfo& D3D12Sampler::GetInfo() const
    {
        return m_createInfo;
    }
}
#endif