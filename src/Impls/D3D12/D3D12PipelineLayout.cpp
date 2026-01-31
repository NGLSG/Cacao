#ifdef WIN32
#include "Impls/D3D12/D3D12PipelineLayout.h"

#include <iostream>
#include <iomanip>
#include <sstream>

#include "Impls/D3D12/D3D12Device.h"
#include "Impls/D3D12/D3D12DescriptorSetLayout.h"
#include <stdexcept>

namespace Cacao
{
    // 调试辅助函数：将 D3D12 类型转为字符串
    static std::string RangeTypeToString(D3D12_DESCRIPTOR_RANGE_TYPE type)
    {
        switch (type)
        {
        case D3D12_DESCRIPTOR_RANGE_TYPE_SRV: return "SRV";
        case D3D12_DESCRIPTOR_RANGE_TYPE_UAV: return "UAV";
        case D3D12_DESCRIPTOR_RANGE_TYPE_CBV: return "CBV";
        case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER: return "SAMPLER";
        default: return "UNKNOWN";
        }
    }

    D3D12PipelineLayout::D3D12PipelineLayout(const Ref<Device>& device, const PipelineLayoutCreateInfo& info)
        : m_createInfo(info)
    {
        if (!device) throw std::runtime_error("D3D12PipelineLayout created with null device");
        m_device = std::dynamic_pointer_cast<D3D12Device>(device);
        if (!m_device) throw std::runtime_error("D3D12PipelineLayout requires a D3D12Device");

        std::vector<D3D12_ROOT_PARAMETER1> rootParameters;
        std::list<std::vector<D3D12_DESCRIPTOR_RANGE1>> descriptorRangeStorage;

        m_setRootParamRange.resize(info.SetLayouts.size());
        uint32_t maxRegisterSpace = 0;

        std::cout << "[D3D12] --- Creating Root Signature ---" << std::endl;

        for (size_t setIndex = 0; setIndex < info.SetLayouts.size(); ++setIndex)
        {
            if (!info.SetLayouts[setIndex]) continue;

            auto* d3d12Layout = dynamic_cast<D3D12DescriptorSetLayout*>(info.SetLayouts[setIndex].get());
            uint32_t startIndex = static_cast<uint32_t>(rootParameters.size());

            const auto& bindings = d3d12Layout->GetBindings();
            if (bindings.empty())
            {
                m_setRootParamRange[setIndex] = {startIndex, startIndex};
                continue;
            }

            if (setIndex >= maxRegisterSpace) maxRegisterSpace = static_cast<uint32_t>(setIndex) + 1;

            std::vector<D3D12_DESCRIPTOR_RANGE1> cbvSrvUavRanges;
            std::vector<D3D12_DESCRIPTOR_RANGE1> samplerRanges;

            for (const auto& binding : bindings)
            {
                D3D12_DESCRIPTOR_RANGE1 range = {};
                // 这里调用 Convert，如果 Convert 返回 UAV，这里就是 UAV
                range.RangeType = D3D12Converter::Convert(binding.Type);
                range.NumDescriptors = binding.Count;
                range.BaseShaderRegister = binding.Binding;
                range.RegisterSpace = static_cast<UINT>(setIndex);
                range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
                range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

                if (range.RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER)
                {
                    samplerRanges.push_back(range);
                }
                else
                {
                    cbvSrvUavRanges.push_back(range);
                }

                // --- 打印调试信息 ---
                std::cout << "  [Set " << setIndex << ", Binding " << binding.Binding << "] Type: "
                    << RangeTypeToString(range.RangeType)
                    << " (Expect SRV for t0/t1)" << std::endl;
            }

            // 创建 CBV/SRV/UAV 表
            if (!cbvSrvUavRanges.empty())
            {
                descriptorRangeStorage.push_back(std::move(cbvSrvUavRanges));
                D3D12_ROOT_PARAMETER1 param = {};
                param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                param.DescriptorTable.NumDescriptorRanges = static_cast<UINT>(descriptorRangeStorage.back().size());
                param.DescriptorTable.pDescriptorRanges = descriptorRangeStorage.back().data();
                param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
                rootParameters.push_back(param);
            }

            // 创建 Sampler 表
            if (!samplerRanges.empty())
            {
                descriptorRangeStorage.push_back(std::move(samplerRanges));
                D3D12_ROOT_PARAMETER1 param = {};
                param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                param.DescriptorTable.NumDescriptorRanges = static_cast<UINT>(descriptorRangeStorage.back().size());
                param.DescriptorTable.pDescriptorRanges = descriptorRangeStorage.back().data();
                param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
                rootParameters.push_back(param);
            }

            m_setRootParamRange[setIndex] = {startIndex, static_cast<uint32_t>(rootParameters.size())};
        }

        // 添加 Push Constants 作为根常量
        if (!info.PushConstantRanges.empty())
        {
            m_pushConstantsRootIndex = static_cast<uint32_t>(rootParameters.size());

            // 计算总大小（以字节为单位）
            uint32_t totalSize = 0;
            for (const auto& range : info.PushConstantRanges)
            {
                uint32_t end = range.Offset + range.Size;
                if (end > totalSize)
                {
                    totalSize = end;
                }
            }

            // 确保大小有效
            if (totalSize > 0)
            {
                D3D12_ROOT_PARAMETER1 param = {};
                param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
                param.Constants.ShaderRegister = 0;
                // 使用不同的 register space 避免冲突
                param.Constants.RegisterSpace = maxRegisterSpace;
                param.Constants.Num32BitValues = (totalSize + 3) / 4; // 转换为 DWORD 数量
                param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

                rootParameters.push_back(param);
            }
        }

        // 验证根签名大小不超过限制 (64 DWORDs)
        uint32_t rootSignatureSizeInDwords = 0;
        for (const auto& param : rootParameters)
        {
            switch (param.ParameterType)
            {
            case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
                rootSignatureSizeInDwords += 1; // 描述符表占用 1 DWORD
                break;
            case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
                rootSignatureSizeInDwords += param.Constants.Num32BitValues;
                break;
            case D3D12_ROOT_PARAMETER_TYPE_CBV:
            case D3D12_ROOT_PARAMETER_TYPE_SRV:
            case D3D12_ROOT_PARAMETER_TYPE_UAV:
                rootSignatureSizeInDwords += 2; // 根描述符占用 2 DWORDs
                break;
            }
        }

        std::cout << "[D3D12] Root signature info: "
            << rootParameters.size() << " parameters, "
            << rootSignatureSizeInDwords << " DWORDs" << std::endl;

        if (rootSignatureSizeInDwords > 64)
        {
            throw std::runtime_error("Root signature exceeds 64 DWORD limit: " +
                std::to_string(rootSignatureSizeInDwords) + " DWORDs");
        }

        // 尝试使用 1. 0 版本的根签名（更好的兼容性）
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

        if (FAILED(m_device->GetHandle()->CheckFeatureSupport(
            D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
        {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        std::cout << "[D3D12] Using root signature version: "
            << (featureData.HighestVersion == D3D_ROOT_SIGNATURE_VERSION_1_1 ? "1. 1" : "1.0")
            << std::endl;

        // 序列化根签名
        ComPtr<ID3DBlob> signatureBlob;
        ComPtr<ID3DBlob> errorBlob;
        HRESULT hr;

        if (featureData.HighestVersion == D3D_ROOT_SIGNATURE_VERSION_1_1)
        {
            // 创建 1.1 版本根签名描述
            D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
            rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
            rootSignatureDesc.Desc_1_1.NumParameters = static_cast<UINT>(rootParameters.size());
            rootSignatureDesc.Desc_1_1.pParameters = rootParameters.empty() ? nullptr : rootParameters.data();
            rootSignatureDesc.Desc_1_1.NumStaticSamplers = 0;
            rootSignatureDesc.Desc_1_1.pStaticSamplers = nullptr;
            rootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

            hr = D3D12SerializeVersionedRootSignature(
                &rootSignatureDesc,
                &signatureBlob,
                &errorBlob);
        }
        else
        {
            // 转换为 1. 0 版本
            std::vector<D3D12_ROOT_PARAMETER> rootParams10;
            std::list<std::vector<D3D12_DESCRIPTOR_RANGE>> rangeStorage10;

            for (const auto& param11 : rootParameters)
            {
                D3D12_ROOT_PARAMETER param10 = {};
                param10.ParameterType = param11.ParameterType;
                param10.ShaderVisibility = param11.ShaderVisibility;

                switch (param11.ParameterType)
                {
                case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
                    {
                        std::vector<D3D12_DESCRIPTOR_RANGE> ranges10;
                        for (UINT i = 0; i < param11.DescriptorTable.NumDescriptorRanges; ++i)
                        {
                            const auto& range11 = param11.DescriptorTable.pDescriptorRanges[i];
                            D3D12_DESCRIPTOR_RANGE range10 = {};
                            range10.RangeType = range11.RangeType;
                            range10.NumDescriptors = range11.NumDescriptors;
                            range10.BaseShaderRegister = range11.BaseShaderRegister;
                            range10.RegisterSpace = range11.RegisterSpace;
                            range10.OffsetInDescriptorsFromTableStart = range11.OffsetInDescriptorsFromTableStart;
                            ranges10.push_back(range10);
                        }
                        rangeStorage10.push_back(std::move(ranges10));
                        param10.DescriptorTable.NumDescriptorRanges =
                            static_cast<UINT>(rangeStorage10.back().size());
                        param10.DescriptorTable.pDescriptorRanges = rangeStorage10.back().data();
                    }
                    break;
                case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
                    param10.Constants = param11.Constants;
                    break;
                case D3D12_ROOT_PARAMETER_TYPE_CBV:
                case D3D12_ROOT_PARAMETER_TYPE_SRV:
                case D3D12_ROOT_PARAMETER_TYPE_UAV:
                    param10.Descriptor.ShaderRegister = param11.Descriptor.ShaderRegister;
                    param10.Descriptor.RegisterSpace = param11.Descriptor.RegisterSpace;
                    break;
                }
                rootParams10.push_back(param10);
            }

            D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
            rootSignatureDesc.NumParameters = static_cast<UINT>(rootParams10.size());
            rootSignatureDesc.pParameters = rootParams10.empty() ? nullptr : rootParams10.data();
            rootSignatureDesc.NumStaticSamplers = 0;
            rootSignatureDesc.pStaticSamplers = nullptr;
            rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

            hr = D3D12SerializeRootSignature(
                &rootSignatureDesc,
                D3D_ROOT_SIGNATURE_VERSION_1_0,
                &signatureBlob,
                &errorBlob);
        }

        if (FAILED(hr))
        {
            std::string errorMsg = "Failed to serialize root signature";
            if (errorBlob)
            {
                errorMsg += ": ";
                errorMsg += static_cast<const char*>(errorBlob->GetBufferPointer());
            }
            std::ostringstream oss;
            oss << errorMsg << " HRESULT: 0x" << std::hex << static_cast<unsigned int>(hr);
            throw std::runtime_error(oss.str());
        }

        // 再次检查设备状态
        auto deviceStatus = m_device->GetHandle()->GetDeviceRemovedReason();
        if (deviceStatus != S_OK)
        {
            std::ostringstream oss;
            oss << "D3D12 device removed before CreateRootSignature.  Reason: 0x"
                << std::hex << static_cast<unsigned int>(deviceStatus);
            throw std::runtime_error(oss.str());
        }

        // 创建根签名
        hr = m_device->GetHandle()->CreateRootSignature(
            0,
            signatureBlob->GetBufferPointer(),
            signatureBlob->GetBufferSize(),
            IID_PPV_ARGS(&m_rootSignature));

        if (FAILED(hr))
        {
            HRESULT removeReason = m_device->GetHandle()->GetDeviceRemovedReason();

            std::ostringstream oss;
            oss << "Failed to create D3D12 root signature. HRESULT: 0x"
                << std::hex << std::setfill('0') << std::setw(8)
                << static_cast<unsigned int>(hr);

            if (removeReason != S_OK)
            {
                oss << ", Device Removed Reason: 0x"
                    << std::hex << std::setfill('0') << std::setw(8)
                    << static_cast<unsigned int>(removeReason);
            }

            std::cerr << oss.str() << std::dec << std::endl;
            throw std::runtime_error("Failed to create D3D12 root signature");
        }

        std::cout << "[D3D12] Root signature created successfully" << std::endl;
    }

    D3D12PipelineLayout::~D3D12PipelineLayout()
    {
        m_rootSignature.Reset();
    }

    uint32_t D3D12PipelineLayout::GetSetRootParamStartIndex(uint32_t setIndex) const
    {
        if (setIndex >= m_setRootParamRange.size())
        {
            throw std::runtime_error("Invalid descriptor set index");
        }
        return m_setRootParamRange[setIndex].first;
    }

    Ref<PipelineLayout> D3D12PipelineLayout::Create(const Ref<Device>& device, const PipelineLayoutCreateInfo& info)
    {
        return CreateRef<D3D12PipelineLayout>(device, info);
    }
}
#endif
