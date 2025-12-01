#include "CacaoCommandBufferEncoder.h"

namespace Cacao
{
    void CacaoCommandBufferEncoder::PipelineBarrier(PipelineStage srcStage, PipelineStage dstStage,
                                                    const std::vector<TextureBarrier>& textureBarriers)
    {
        PipelineBarrier(srcStage, dstStage, {}, {}, textureBarriers);
    }

    void CacaoCommandBufferEncoder::PipelineBarrier(PipelineStage srcStage, PipelineStage dstStage,
                                                    const std::vector<BufferBarrier>& bufferBarriers)
    {
        PipelineBarrier(srcStage, dstStage, {}, bufferBarriers, {});
    }
}
