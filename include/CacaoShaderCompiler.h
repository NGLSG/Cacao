#ifndef CACAO_CACAOSHADERCOMPILER_H
#define CACAO_CACAOSHADERCOMPILER_H
#include <slang/slang.h>
#include "CacaoShaderModule.h"
#include "slang-com-ptr.h"
using namespace slang;
namespace Cacao
{
    enum class CacaoType;
    struct ShaderCreateInfo;
    class CacaoShaderModule;
    class CacaoDevice;
    class CACAO_API CacaoShaderCompiler
    {
    public:
        CacaoShaderCompiler();
        ~CacaoShaderCompiler();
        static Ref<CacaoShaderCompiler> Create(CacaoType backend);
        void Initialize(CacaoType backend);
        std::shared_ptr<CacaoShaderModule> CompileOrLoad(
            const Ref<CacaoDevice>& device,
            const ShaderCreateInfo& info);
        void SetCacheDirectory(const std::filesystem::path& path);
        void PruneCache();
    private:
        size_t CalculateHash(const ShaderCreateInfo& info) const;
        ShaderBlob ConvertBlob(slang::IBlob* blob);
        SlangStage ConvertShaderStageToSlang(ShaderStage stage);
        CacaoType m_targetBackend;
        std::filesystem::path m_cacheDir = "shader_cache";
        Slang::ComPtr<slang::IGlobalSession> m_globalSession;
        Slang::ComPtr<slang::ISession> m_session;
    };
}
#endif 
