#ifndef CACAO_CACAOSHADERCOMPILER_H
#define CACAO_CACAOSHADERCOMPILER_H
#include <slang/slang.h>
#include "ShaderModule.h"
#include "slang-com-ptr.h"
using namespace slang;
namespace Cacao
{
    enum class BackendType;
    struct ShaderCreateInfo;
    class ShaderModule;
    class Device;
    class CACAO_API ShaderCompiler
    {
    public:
        ShaderCompiler();
        ~ShaderCompiler();
        static Ref<ShaderCompiler> Create(BackendType backend);
        void Initialize(BackendType backend);
        std::shared_ptr<ShaderModule> CompileOrLoad(
            const Ref<Device>& device,
            const ShaderCreateInfo& info);
        void SetCacheDirectory(const std::filesystem::path& path);
        void PruneCache();
    private:
        size_t CalculateHash(const ShaderCreateInfo& info) const;
        ShaderBlob ConvertBlob(slang::IBlob* blob);
        SlangStage ConvertShaderStageToSlang(ShaderStage stage);
        BackendType m_targetBackend;
        std::filesystem::path m_cacheDir = "shader_cache";
        Slang::ComPtr<slang::IGlobalSession> m_globalSession;
        Slang::ComPtr<slang::ISession> m_session;
    };
}
#endif 
