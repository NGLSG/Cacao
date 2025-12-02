#ifndef CACAO_D3D12DESCRIPTORSET_H
#define CACAO_D3D12DESCRIPTORSET_H
#ifdef WIN32
#include "DescriptorSet.h"
#include "D3D12Common.h"
namespace Cacao
{
    class CACAO_API D3D12DescriptorSet : public DescriptorSet
    {
    public:
        void WriteBuffer(const BufferWriteInfo& info) override;
        void WriteBuffers(const BufferWriteInfos& infos) override;
        void WriteTexture(const TextureWriteInfo& info) override;
        void WriteTextures(const TextureWriteInfos& infos) override;
        void WriteSampler(const SamplerWriteInfo& info) override;
        void WriteSamplers(const SamplerWriteInfos& infos) override;
        void WriteAccelerationStructure(const AccelerationStructureWriteInfo& info) override;
        void WriteAccelerationStructures(const AccelerationStructureWriteInfos& infos) override;
        void Update() override;
    };
}
#endif
#endif 
