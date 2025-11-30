#ifndef CACAO_CACAOQUEUE_H
#define CACAO_CACAOQUEUE_H
namespace Cacao
{
    class CacaoSynchronization;
    class CacaoCommandBufferEncoder;
    enum class QueueType;
    class CACAO_API CacaoQueue : public std::enable_shared_from_this<CacaoQueue>
    {
    public:
        virtual ~CacaoQueue() = default;
        virtual QueueType GetType() const = 0;
        virtual uint32_t GetIndex() const = 0;
        virtual void Submit(
            const Ref<CacaoCommandBufferEncoder>& cmd,
            const Ref<CacaoSynchronization>& sync,
            uint32_t frameIndex) = 0;
        virtual void Submit(
            const std::vector<Ref<CacaoCommandBufferEncoder>>& cmds,
            const Ref<CacaoSynchronization>& sync,
            uint32_t frameIndex) = 0;
        virtual void Submit(const Ref<CacaoCommandBufferEncoder>& cmd) = 0;
        virtual void WaitIdle() const = 0;
    };
} 
#endif 
