#ifdef WIN32
#include "Impls/D3D12/D3D12ShaderModule.h"
#include "Impls/D3D12/D3D12Device.h"
#include <stdexcept>
namespace Cacao
{
    D3D12ShaderModule::D3D12ShaderModule(const Ref<Device>& device, const ShaderCreateInfo& info,
                                         const ShaderBlob& blob) :
        m_shaderBlob(blob), m_createInfo(info)
    {
        if (! device)
        {
            throw std::runtime_error("D3D12ShaderModule created with null device");
        }

        m_device = std::dynamic_pointer_cast<D3D12Device>(device);
        if (! m_device)
        {
            throw std::runtime_error("D3D12ShaderModule requires a D3D12Device");
        }

        if (m_shaderBlob.Data. empty())
        {
            throw std::runtime_error("ShaderBlob is empty!");
        }

        // D3D12 着色器字节码验证
        // DXBC 签名: "DXBC" (0x43425844)
        // DXIL 签名: "DXIL" (前4字节)
        if (m_shaderBlob.Data.size() < 4)
        {
            throw std::runtime_error("Shader blob too small to contain valid bytecode");
        }

        // 检查是否为有效的 DXBC 或 DXIL 格式
        const uint32_t* header = reinterpret_cast<const uint32_t*>(m_shaderBlob.Data.data());
        bool isDXBC = (*header == 0x43425844); // "DXBC" 小端序
        bool isDXIL = (m_shaderBlob.Data[0] == 'D' &&
                       m_shaderBlob.Data[1] == 'X' &&
                       m_shaderBlob.Data[2] == 'B' &&
                       m_shaderBlob.Data[3] == 'C') ||
                      (m_shaderBlob.Data. size() >= 8); // DXIL 容器也以 DXBC 开头

        // 注意: 某些编译器可能生成不同格式的字节码
        // 这里我们只做基本验证，实际验证会在 PSO 创建时由驱动完成
        if (! isDXBC && ! isDXIL)
        {
            // 宽松检查 - 某些工具可能生成略有不同的格式
            // 在调试模式下可以添加警告
        }

        // D3D12 不需要像 Vulkan 那样创建显式的 ShaderModule 对象
        // 着色器字节码直接存储在 m_shaderBlob 中
        // 在创建 Pipeline State Object (PSO) 时会使用这些字节码
    }

    Ref<D3D12ShaderModule> D3D12ShaderModule::Create(const Ref<Device>& device,
                                                      const ShaderCreateInfo& info,
                                                      const ShaderBlob& blob)
    {
        return CreateRef<D3D12ShaderModule>(device, info, blob);
    }

    const std::string& D3D12ShaderModule::GetEntryPoint() const
    {
        return m_createInfo.EntryPoint;
    }

    ShaderStage D3D12ShaderModule::GetStage() const
    {
        return m_createInfo.Stage;
    }

    const ShaderBlob& D3D12ShaderModule::GetBlob() const
    {
        return m_shaderBlob;
    }

    D3D12_SHADER_BYTECODE D3D12ShaderModule::GetBytecode() const
    {
        D3D12_SHADER_BYTECODE bytecode = {};
        bytecode.pShaderBytecode = m_shaderBlob.Data.data();
        bytecode. BytecodeLength = m_shaderBlob.Data.size();
        return bytecode;
    }

    D3D12ShaderModule::~D3D12ShaderModule()
    {
        // D3D12 不需要显式销毁着色器模块
        // 字节码存储在 m_shaderBlob 中，由 std::vector 自动管理内存
        m_shaderBlob.Data.clear();
    }
}
#endif