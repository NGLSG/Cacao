#ifndef CACAO_CACAOSYNCHRONIZATION_H
#define CACAO_CACAOSYNCHRONIZATION_H
namespace Cacao
{
    class CacaoSwapchain;
}
namespace Cacao
{
    class CACAO_API CacaoSynchronization : public std::enable_shared_from_this<CacaoSynchronization>
    {
    public:
        virtual void WaitForFrame(uint32_t frameIndex) const =0;
        virtual void ResetFrameFence(uint32_t frameIndex) const =0;
        virtual uint32_t AcquireNextImageIndex(const Ref<CacaoSwapchain>& swapchain, uint32_t frameIndex) const =0;
        virtual uint32_t GetMaxFramesInFlight() const =0;
        virtual ~CacaoSynchronization() = default;
    };
}
#endif 
