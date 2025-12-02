#ifndef CACAO_CACAOSYNCHRONIZATION_H
#define CACAO_CACAOSYNCHRONIZATION_H
namespace Cacao
{
    class Swapchain;
    class CACAO_API Synchronization : public std::enable_shared_from_this<Synchronization>
    {
    public:
        virtual void WaitForFrame(uint32_t frameIndex) const =0;
        virtual void ResetFrameFence(uint32_t frameIndex) const =0;
        virtual uint32_t AcquireNextImageIndex(const Ref<Swapchain>& swapchain, uint32_t frameIndex) const =0;
        virtual uint32_t GetMaxFramesInFlight() const =0;
        virtual ~Synchronization() = default;
    };
}
#endif 
