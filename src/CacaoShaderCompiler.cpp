#include "CacaoShaderCompiler.h"
#include <iostream>
#include <fstream>
#include "CacaoDevice.h"
#include "CacaoInstance.h"
namespace Cacao
{
    CacaoShaderCompiler::CacaoShaderCompiler()
    {
    }
    CacaoShaderCompiler::~CacaoShaderCompiler()
    {
    }
    Ref<CacaoShaderCompiler> CacaoShaderCompiler::Create(CacaoType backend)
    {
        auto compiler = CreateRef<CacaoShaderCompiler>();
        compiler->Initialize(backend);
        return compiler;
    }
    void CacaoShaderCompiler::Initialize(CacaoType backend)
    {
        m_targetBackend = backend;
        if (SLANG_FAILED(createGlobalSession(m_globalSession. writeRef())))
        {
            throw std::runtime_error("Failed to create Slang global session");
        }
        SessionDesc sessionDesc = {};
        TargetDesc targetDesc = {};
        switch (backend)
        {
        case CacaoType::Vulkan:
            targetDesc.format = SLANG_SPIRV;
            targetDesc. profile = m_globalSession->findProfile("spirv_1_5");
            break;
        case CacaoType::DirectX12:
            targetDesc.format = SLANG_DXIL;
            targetDesc.profile = m_globalSession->findProfile("sm_6_5");
            break;
        case CacaoType::OpenGL:
        case CacaoType::OpenGLES:
            targetDesc.format = SLANG_GLSL;
            targetDesc.profile = m_globalSession->findProfile("glsl_450");
            break;
        case CacaoType::DirectX11:
            targetDesc.format = SLANG_HLSL;
            targetDesc.profile = m_globalSession->findProfile("sm_5_1");
            break;
        case CacaoType::Metal:
            targetDesc.format = SLANG_METAL;
            targetDesc.profile = m_globalSession->findProfile("metal");
            break;
        case CacaoType::WebGPU:
            targetDesc.format = SLANG_WGSL;
            break;
        default:
            throw std::runtime_error("Unsupported CacaoType in CacaoShaderCompiler");
        }
        sessionDesc. targetCount = 1;
        sessionDesc. targets = &targetDesc;
        if (SLANG_FAILED(m_globalSession->createSession(sessionDesc, m_session.writeRef())))
        {
            throw std::runtime_error("Failed to create Slang session");
        }
    }
    std::shared_ptr<CacaoShaderModule> CacaoShaderCompiler::CompileOrLoad(
        const Ref<CacaoDevice>& device, const ShaderCreateInfo& info)
    {
        size_t hash = CalculateHash(info);
        std::filesystem::path cachePath = m_cacheDir / (std::to_string(hash) + ".bin");
        ShaderBlob blob;
        if (std::filesystem::exists(cachePath))
        {
            std::ifstream ifs(cachePath, std::ios::binary | std::ios::ate);
            if (! ifs)
            {
                throw std::runtime_error("Failed to open shader cache file: " + cachePath. string());
            }
            std::streamsize size = ifs.tellg();
            ifs.seekg(0, std::ios::beg);
            blob.Data.resize(size);
            if (! ifs.read(reinterpret_cast<char*>(blob.Data.data()), size))
            {
                throw std::runtime_error("Failed to read shader cache file: " + cachePath.string());
            }
            blob.Hash = std::hash<std::string_view>()(
                std::string_view(reinterpret_cast<const char*>(blob.Data.data()), blob.Data.size())
            );
            return device->CreateShaderModule(blob, info);
        }
        Slang::ComPtr<slang::IBlob> diagnosticsBlob;
        Slang::ComPtr<slang::IModule> slangModule;
        slangModule = m_session->loadModule(info.SourcePath. c_str(), diagnosticsBlob.writeRef());
        if (diagnosticsBlob && diagnosticsBlob->getBufferSize() > 0)
        {
            std::cerr << "Shader load diagnostics:\n"
                      << static_cast<const char*>(diagnosticsBlob->getBufferPointer()) << std::endl;
        }
        if (! slangModule)
        {
            std::cerr << "Failed to load shader module: " << info.SourcePath << std::endl;
            return nullptr;
        }
        Slang::ComPtr<slang::IEntryPoint> entryPoint;
        SlangStage slangStage = ConvertShaderStageToSlang(info.Stage);
        SlangResult result = slangModule->findEntryPointByName(info. EntryPoint.c_str(), entryPoint.writeRef());
        if (SLANG_FAILED(result) || !entryPoint)
        {
            std::cerr << "Failed to find entry point: " << info.EntryPoint << std::endl;
            return nullptr;
        }
        std::vector<slang::IComponentType*> components;
        components.push_back(slangModule);
        components.push_back(entryPoint);
        Slang::ComPtr<slang::IComponentType> composedProgram;
        result = m_session->createCompositeComponentType(
            components.data(),
            static_cast<SlangInt>(components. size()),
            composedProgram.writeRef(),
            diagnosticsBlob.writeRef());
        if (diagnosticsBlob && diagnosticsBlob->getBufferSize() > 0)
        {
            std::cerr << "Composition diagnostics:\n"
                      << static_cast<const char*>(diagnosticsBlob->getBufferPointer()) << std::endl;
        }
        if (SLANG_FAILED(result) || !composedProgram)
        {
            std::cerr << "Failed to create composite program" << std::endl;
            return nullptr;
        }
        Slang::ComPtr<slang::IComponentType> linkedProgram;
        result = composedProgram->link(linkedProgram. writeRef(), diagnosticsBlob. writeRef());
        if (diagnosticsBlob && diagnosticsBlob->getBufferSize() > 0)
        {
            std::cerr << "Link diagnostics:\n"
                      << static_cast<const char*>(diagnosticsBlob->getBufferPointer()) << std::endl;
        }
        if (SLANG_FAILED(result) || !linkedProgram)
        {
            std::cerr << "Failed to link program" << std::endl;
            return nullptr;
        }
        Slang::ComPtr<slang::IBlob> codeBlob;
        result = linkedProgram->getEntryPointCode(
            0,  
            0,  
            codeBlob.writeRef(),
            diagnosticsBlob.writeRef());
        if (diagnosticsBlob && diagnosticsBlob->getBufferSize() > 0)
        {
            std::cerr << "Code generation diagnostics:\n"
                      << static_cast<const char*>(diagnosticsBlob->getBufferPointer()) << std::endl;
        }
        if (SLANG_FAILED(result) || !codeBlob)
        {
            std::cerr << "Failed to get entry point code" << std::endl;
            return nullptr;
        }
        blob = ConvertBlob(codeBlob. get());
        if (! blob.Data.empty())
        {
            if (!std::filesystem::exists(m_cacheDir))
            {
                std::filesystem::create_directories(m_cacheDir);
            }
            std::ofstream ofs(cachePath, std::ios::binary);
            if (ofs)
            {
                ofs.write(reinterpret_cast<const char*>(blob.Data. data()),
                          static_cast<std::streamsize>(blob. Data.size()));
                ofs.close();
            }
        }
        return device->CreateShaderModule(blob, info);
    }
    void CacaoShaderCompiler::SetCacheDirectory(const std::filesystem::path& path)
    {
        m_cacheDir = path;
        if (! std::filesystem::exists(m_cacheDir))
        {
            std::filesystem::create_directories(m_cacheDir);
        }
    }
    void CacaoShaderCompiler::PruneCache()
    {
        if (std::filesystem::exists(m_cacheDir))
        {
            for (const auto& entry : std::filesystem::directory_iterator(m_cacheDir))
            {
                std::filesystem::remove(entry.path());
            }
        }
    }
    size_t CacaoShaderCompiler::CalculateHash(const ShaderCreateInfo& info) const
    {
        std::string combined = info.SourcePath + "|" + info.EntryPoint + "|" + info.Profile;
        combined += "|" + std::to_string(static_cast<int>(info.Stage));
        combined += "|" + std::to_string(static_cast<int>(m_targetBackend));
        for (const auto& define : info.Defines)
        {
            combined += "|" + define.first + "=" + define.second;
        }
        return std::hash<std::string>()(combined);
    }
    ShaderBlob CacaoShaderCompiler::ConvertBlob(IBlob* blob)
    {
        ShaderBlob result;
        if (!blob)
        {
            return result;
        }
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(blob->getBufferPointer());
        size_t size = blob->getBufferSize();
        if (!ptr || size == 0)
        {
            return result;
        }
        result.Data. assign(ptr, ptr + size);
        result.Hash = std::hash<std::string_view>()(
            std::string_view(reinterpret_cast<const char*>(ptr), size)
        );
        return result;
    }
    SlangStage CacaoShaderCompiler::ConvertShaderStageToSlang(ShaderStage stage)
    {
        switch (stage)
        {
        case ShaderStage::Vertex:
            return SLANG_STAGE_VERTEX;
        case ShaderStage::Fragment:
            return SLANG_STAGE_FRAGMENT;
        case ShaderStage::Compute:
            return SLANG_STAGE_COMPUTE;
        case ShaderStage::Geometry:
            return SLANG_STAGE_GEOMETRY;
        case ShaderStage::TessellationControl:
            return SLANG_STAGE_HULL;
        case ShaderStage::TessellationEvaluation:
            return SLANG_STAGE_DOMAIN;
        default:
            return SLANG_STAGE_NONE;
        }
    }
}
