#ifndef CACAO_CACAOADAPTER_H
#define CACAO_CACAOADAPTER_H
namespace Cacao
{
    struct DeviceCreateInfo;
    class Device;
    enum class AdapterType
    {
        Discrete,
        Integrated,
        Software,
        Unknown
    };
    struct AdapterProperties
    {
        uint32_t deviceID;
        uint32_t vendorID;
        std::string name;
        AdapterType type;
        uint64_t dedicatedVideoMemory;
    };
    enum class QueueType
    {
        Graphics,
        Compute,
        Transfer,
        Present 
    };
    enum class CacaoFeature : uint32_t
    {
        GeometryShader,
        TessellationShader,
        MultiViewport,
        IndependentBlending,
        RayTracing,
        MeshShader,
        VariableRateShading,
        BindlessDescriptors,
        BufferDeviceAddress,
        DrawIndirectCount,
        ShaderFloat16,
        ShaderInt64,
        PipelineStatistics,
    };
    class CACAO_API Adapter : public std::enable_shared_from_this<Adapter>
    {
    public :
        virtual ~Adapter() = default;
        virtual AdapterProperties GetProperties() const = 0;
        virtual AdapterType GetAdapterType() const = 0;
        virtual bool IsFeatureSupported(CacaoFeature feature) const = 0;
        virtual std::shared_ptr<Device> CreateDevice(const DeviceCreateInfo& info) = 0;
        virtual uint32_t FindQueueFamilyIndex(QueueType type) const = 0;
    };
    template <>
    struct to_string<AdapterType>
    {
        static std::string convert(AdapterType type)
        {
            switch (type)
            {
            case AdapterType::Discrete:
                return "Discrete";
            case AdapterType::Integrated:
                return "Integrated";
            case AdapterType::Software:
                return "Software";
            case AdapterType::Unknown:
            default:
                return "Unknown";
            }
        }
    };
}
#endif
