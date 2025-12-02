#pragma once
#ifndef D3D12MA_D3D12_HEADERS_ALREADY_INCLUDED
    #if defined(D3D12MA_USING_DIRECTX_HEADERS)
        #include <directx/d3d12.h>
        #include <dxguids/dxguids.h>
    #else
        #include <d3d12.h>
    #endif
    #include <dxgi1_4.h>
#endif
#ifndef D3D12MA_DXGI_1_4
    #ifdef __IDXGIAdapter3_INTERFACE_DEFINED__
        #define D3D12MA_DXGI_1_4 1
    #else
        #define D3D12MA_DXGI_1_4 0
    #endif
#endif
#ifndef D3D12MA_CREATE_NOT_ZEROED_AVAILABLE
    #ifdef __ID3D12Device8_INTERFACE_DEFINED__
        #define D3D12MA_CREATE_NOT_ZEROED_AVAILABLE 1
    #else
        #define D3D12MA_CREATE_NOT_ZEROED_AVAILABLE 0
    #endif
#endif
#ifndef D3D12MA_USE_SMALL_RESOURCE_PLACEMENT_ALIGNMENT
    #define D3D12MA_USE_SMALL_RESOURCE_PLACEMENT_ALIGNMENT 1
#endif
#ifndef D3D12MA_RECOMMENDED_ALLOCATOR_FLAGS
    #define D3D12MA_RECOMMENDED_ALLOCATOR_FLAGS (D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED | D3D12MA::ALLOCATOR_FLAG_MSAA_TEXTURES_ALWAYS_COMMITTED)
#endif
#ifndef D3D12MA_RECOMMENDED_HEAP_FLAGS
    #if D3D12MA_CREATE_NOT_ZEROED_AVAILABLE
        #define D3D12MA_RECOMMENDED_HEAP_FLAGS (D3D12_HEAP_FLAG_CREATE_NOT_ZEROED)
    #else
        #define D3D12MA_RECOMMENDED_HEAP_FLAGS (D3D12_HEAP_FLAG_NONE)
    #endif
#endif
#ifndef D3D12MA_RECOMMENDED_POOL_FLAGS
    #define D3D12MA_RECOMMENDED_POOL_FLAGS (D3D12MA::POOL_FLAG_MSAA_TEXTURES_ALWAYS_COMMITTED)
#endif
#define D3D12MA_CLASS_NO_COPY(className) \
    private: \
        className(const className&) = delete; \
        className(className&&) = delete; \
        className& operator=(const className&) = delete; \
        className& operator=(className&&) = delete;
#define FACILITY_D3D12MA 3542
#if !defined(D3D12MA_ATOMIC_UINT32) || !defined(D3D12MA_ATOMIC_UINT64)
    #include <atomic>
#endif
#ifndef D3D12MA_ATOMIC_UINT32
    #define D3D12MA_ATOMIC_UINT32 std::atomic<UINT>
#endif
#ifndef D3D12MA_ATOMIC_UINT64
    #define D3D12MA_ATOMIC_UINT64 std::atomic<UINT64>
#endif
#ifdef D3D12MA_EXPORTS
    #define D3D12MA_API __declspec(dllexport)
#elif defined(D3D12MA_IMPORTS)
    #define D3D12MA_API __declspec(dllimport)
#else
    #define D3D12MA_API
#endif
struct ID3D12ProtectedResourceSession;
#ifndef __ID3D12Device1_INTERFACE_DEFINED__
typedef enum D3D12_RESIDENCY_PRIORITY
{
    D3D12_RESIDENCY_PRIORITY_MINIMUM = 0x28000000,
    D3D12_RESIDENCY_PRIORITY_LOW = 0x50000000,
    D3D12_RESIDENCY_PRIORITY_NORMAL = 0x78000000,
    D3D12_RESIDENCY_PRIORITY_HIGH = 0xa0010000,
    D3D12_RESIDENCY_PRIORITY_MAXIMUM = 0xc8000000
} D3D12_RESIDENCY_PRIORITY;
#endif
namespace D3D12MA
{
class D3D12MA_API IUnknownImpl : public IUnknown
{
public:
    virtual ~IUnknownImpl() = default;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;
protected:
    virtual void ReleaseThis() { delete this; }
private:
    D3D12MA_ATOMIC_UINT32 m_RefCount = {1};
};
} 
namespace D3D12MA
{
class DefragmentationContextPimpl;
class AllocatorPimpl;
class PoolPimpl;
class NormalBlock;
class BlockVector;
class CommittedAllocationList;
class JsonWriter;
class VirtualBlockPimpl;
class Pool;
class Allocator;
struct Statistics;
struct DetailedStatistics;
struct TotalStatistics;
typedef UINT64 AllocHandle;
using ALLOCATE_FUNC_PTR = void* (*)(size_t Size, size_t Alignment, void* pPrivateData);
using FREE_FUNC_PTR = void (*)(void* pMemory, void* pPrivateData);
struct ALLOCATION_CALLBACKS
{
    ALLOCATE_FUNC_PTR pAllocate;
    FREE_FUNC_PTR pFree;
    void* pPrivateData;
};
enum ALLOCATION_FLAGS
{
    ALLOCATION_FLAG_NONE = 0,
    ALLOCATION_FLAG_COMMITTED = 0x1,
    ALLOCATION_FLAG_NEVER_ALLOCATE = 0x2,
    ALLOCATION_FLAG_WITHIN_BUDGET = 0x4,
    ALLOCATION_FLAG_UPPER_ADDRESS = 0x8,
    ALLOCATION_FLAG_CAN_ALIAS = 0x10,
    ALLOCATION_FLAG_STRATEGY_MIN_MEMORY = 0x00010000,
    ALLOCATION_FLAG_STRATEGY_MIN_TIME = 0x00020000,
    ALLOCATION_FLAG_STRATEGY_MIN_OFFSET = 0x0004000,
    ALLOCATION_FLAG_STRATEGY_BEST_FIT = ALLOCATION_FLAG_STRATEGY_MIN_MEMORY,
    ALLOCATION_FLAG_STRATEGY_FIRST_FIT = ALLOCATION_FLAG_STRATEGY_MIN_TIME,
    ALLOCATION_FLAG_STRATEGY_MASK =
        ALLOCATION_FLAG_STRATEGY_MIN_MEMORY |
        ALLOCATION_FLAG_STRATEGY_MIN_TIME |
        ALLOCATION_FLAG_STRATEGY_MIN_OFFSET,
};
struct ALLOCATION_DESC
{
    ALLOCATION_FLAGS Flags;
    D3D12_HEAP_TYPE HeapType;
    D3D12_HEAP_FLAGS ExtraHeapFlags;
    Pool* CustomPool;
    void* pPrivateData;
};
struct Statistics
{
    UINT BlockCount;
    UINT AllocationCount;
    UINT64 BlockBytes;
    UINT64 AllocationBytes;
};
struct DetailedStatistics
{
    Statistics Stats;
    UINT UnusedRangeCount;
    UINT64 AllocationSizeMin;
    UINT64 AllocationSizeMax;
    UINT64 UnusedRangeSizeMin;
    UINT64 UnusedRangeSizeMax;
};
struct TotalStatistics
{
    DetailedStatistics HeapType[5];
    DetailedStatistics MemorySegmentGroup[2];
    DetailedStatistics Total;
};
struct Budget
{
    Statistics Stats;
    UINT64 UsageBytes;
    UINT64 BudgetBytes;
};
struct D3D12MA_API VirtualAllocation
{
    AllocHandle AllocHandle;
};
class D3D12MA_API Allocation : public IUnknownImpl
{
public:
    UINT64 GetOffset() const;
    UINT64 GetAlignment() const { return m_Alignment; }
    UINT64 GetSize() const { return m_Size; }
    ID3D12Resource* GetResource() const { return m_Resource; }
    void SetResource(ID3D12Resource* pResource);
    ID3D12Heap* GetHeap() const;
    void SetPrivateData(void* pPrivateData) { m_pPrivateData = pPrivateData; }
    void* GetPrivateData() const { return m_pPrivateData; }
    void SetName(LPCWSTR Name);
    LPCWSTR GetName() const { return m_Name; }
protected:
    void ReleaseThis() override;
private:
    friend class AllocatorPimpl;
    friend class BlockVector;
    friend class CommittedAllocationList;
    friend class JsonWriter;
    friend class BlockMetadata_Linear;
    friend class DefragmentationContextPimpl;
    friend struct CommittedAllocationListItemTraits;
    template<typename T> friend void D3D12MA_DELETE(const ALLOCATION_CALLBACKS&, T*);
    template<typename T> friend class PoolAllocator;
    enum Type
    {
        TYPE_COMMITTED,
        TYPE_PLACED,
        TYPE_HEAP,
        TYPE_COUNT
    };
    AllocatorPimpl* m_Allocator;
    UINT64 m_Size;
    UINT64 m_Alignment;
    ID3D12Resource* m_Resource;
    void* m_pPrivateData;
    wchar_t* m_Name;
    union
    {
        struct
        {
            CommittedAllocationList* list;
            Allocation* prev;
            Allocation* next;
        } m_Committed;
        struct
        {
            AllocHandle allocHandle;
            NormalBlock* block;
        } m_Placed;
        struct
        {
            CommittedAllocationList* list;
            Allocation* prev;
            Allocation* next;
            ID3D12Heap* heap;
        } m_Heap;
    };
    struct PackedData
    {
    public:
        PackedData() :
            m_Type(0), m_ResourceDimension(0), m_ResourceFlags(0), m_TextureLayout(0) { }
        Type GetType() const { return (Type)m_Type; }
        D3D12_RESOURCE_DIMENSION GetResourceDimension() const { return (D3D12_RESOURCE_DIMENSION)m_ResourceDimension; }
        D3D12_RESOURCE_FLAGS GetResourceFlags() const { return (D3D12_RESOURCE_FLAGS)m_ResourceFlags; }
        D3D12_TEXTURE_LAYOUT GetTextureLayout() const { return (D3D12_TEXTURE_LAYOUT)m_TextureLayout; }
        void SetType(Type type);
        void SetResourceDimension(D3D12_RESOURCE_DIMENSION resourceDimension);
        void SetResourceFlags(D3D12_RESOURCE_FLAGS resourceFlags);
        void SetTextureLayout(D3D12_TEXTURE_LAYOUT textureLayout);
    private:
        UINT m_Type : 2;               
        UINT m_ResourceDimension : 3;  
        UINT m_ResourceFlags : 24;     
        UINT m_TextureLayout : 9;      
    } m_PackedData;
    Allocation(AllocatorPimpl* allocator, UINT64 size, UINT64 alignment);
    virtual ~Allocation() = default;
    void InitCommitted(CommittedAllocationList* list);
    void InitPlaced(AllocHandle allocHandle, NormalBlock* block);
    void InitHeap(CommittedAllocationList* list, ID3D12Heap* heap);
    void SwapBlockAllocation(Allocation* allocation);
    AllocHandle GetAllocHandle() const;
    NormalBlock* GetBlock();
    template<typename D3D12_RESOURCE_DESC_T>
    void SetResourcePointer(ID3D12Resource* resource, const D3D12_RESOURCE_DESC_T* pResourceDesc);
    void FreeName();
    D3D12MA_CLASS_NO_COPY(Allocation)
};
enum DEFRAGMENTATION_FLAGS
{
    DEFRAGMENTATION_FLAG_ALGORITHM_FAST = 0x1,
    DEFRAGMENTATION_FLAG_ALGORITHM_BALANCED = 0x2,
    DEFRAGMENTATION_FLAG_ALGORITHM_FULL = 0x4,
    DEFRAGMENTATION_FLAG_ALGORITHM_MASK =
        DEFRAGMENTATION_FLAG_ALGORITHM_FAST |
        DEFRAGMENTATION_FLAG_ALGORITHM_BALANCED |
        DEFRAGMENTATION_FLAG_ALGORITHM_FULL
};
struct DEFRAGMENTATION_DESC
{
    DEFRAGMENTATION_FLAGS Flags;
    UINT64 MaxBytesPerPass;
    UINT32 MaxAllocationsPerPass;
};
enum DEFRAGMENTATION_MOVE_OPERATION
{
    DEFRAGMENTATION_MOVE_OPERATION_COPY = 0,
    DEFRAGMENTATION_MOVE_OPERATION_IGNORE = 1,
    DEFRAGMENTATION_MOVE_OPERATION_DESTROY = 2,
};
struct DEFRAGMENTATION_MOVE
{
    DEFRAGMENTATION_MOVE_OPERATION Operation;
    Allocation* pSrcAllocation;
    Allocation* pDstTmpAllocation;
};
struct DEFRAGMENTATION_PASS_MOVE_INFO
{
    UINT32 MoveCount;
    DEFRAGMENTATION_MOVE* pMoves;
};
struct DEFRAGMENTATION_STATS
{
    UINT64 BytesMoved;
    UINT64 BytesFreed;
    UINT32 AllocationsMoved;
    UINT32 HeapsFreed;
};
class D3D12MA_API DefragmentationContext : public IUnknownImpl
{
public:
    HRESULT BeginPass(DEFRAGMENTATION_PASS_MOVE_INFO* pPassInfo);
    HRESULT EndPass(DEFRAGMENTATION_PASS_MOVE_INFO* pPassInfo);
    void GetStats(DEFRAGMENTATION_STATS* pStats);
protected:
    void ReleaseThis() override;
private:
    friend class Pool;
    friend class Allocator;
    template<typename T> friend void D3D12MA_DELETE(const ALLOCATION_CALLBACKS&, T*);
    DefragmentationContextPimpl* m_Pimpl;
    DefragmentationContext(AllocatorPimpl* allocator,
        const DEFRAGMENTATION_DESC& desc,
        BlockVector* poolVector);
    ~DefragmentationContext();
    D3D12MA_CLASS_NO_COPY(DefragmentationContext)
};
enum POOL_FLAGS
{
    POOL_FLAG_NONE = 0,
    POOL_FLAG_ALGORITHM_LINEAR = 0x1,
    POOL_FLAG_MSAA_TEXTURES_ALWAYS_COMMITTED = 0x2,
    POOL_FLAG_ALWAYS_COMMITTED = 0x4,
    POOL_FLAG_ALGORITHM_MASK = POOL_FLAG_ALGORITHM_LINEAR
};
struct POOL_DESC
{
    POOL_FLAGS Flags;
    D3D12_HEAP_PROPERTIES HeapProperties;
    D3D12_HEAP_FLAGS HeapFlags;
    UINT64 BlockSize;
    UINT MinBlockCount;
    UINT MaxBlockCount;
    UINT64 MinAllocationAlignment;
    ID3D12ProtectedResourceSession* pProtectedSession;
    D3D12_RESIDENCY_PRIORITY ResidencyPriority;
};
class D3D12MA_API Pool : public IUnknownImpl
{
public:
    POOL_DESC GetDesc() const;
    void GetStatistics(Statistics* pStats);
    void CalculateStatistics(DetailedStatistics* pStats);
    void SetName(LPCWSTR Name);
    LPCWSTR GetName() const;
    HRESULT BeginDefragmentation(const DEFRAGMENTATION_DESC* pDesc, DefragmentationContext** ppContext);
protected:
    void ReleaseThis() override;
private:
    friend class Allocator;
    friend class AllocatorPimpl;
    template<typename T> friend void D3D12MA_DELETE(const ALLOCATION_CALLBACKS&, T*);
    PoolPimpl* m_Pimpl;
    Pool(Allocator* allocator, const POOL_DESC &desc);
    ~Pool();
    D3D12MA_CLASS_NO_COPY(Pool)
};
enum ALLOCATOR_FLAGS
{
    ALLOCATOR_FLAG_NONE = 0,
    ALLOCATOR_FLAG_SINGLETHREADED = 0x1,
    ALLOCATOR_FLAG_ALWAYS_COMMITTED = 0x2,
    ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED = 0x4,
    ALLOCATOR_FLAG_MSAA_TEXTURES_ALWAYS_COMMITTED = 0x8,
    ALLOCATOR_FLAG_DONT_PREFER_SMALL_BUFFERS_COMMITTED = 0x10,
    ALLOCATOR_FLAG_DONT_USE_TIGHT_ALIGNMENT = 0x20,
};
struct ALLOCATOR_DESC
{
    ALLOCATOR_FLAGS Flags;
    ID3D12Device* pDevice;
    UINT64 PreferredBlockSize;
    const ALLOCATION_CALLBACKS* pAllocationCallbacks;
    IDXGIAdapter* pAdapter;
};
class D3D12MA_API Allocator : public IUnknownImpl
{
public:
    const D3D12_FEATURE_DATA_D3D12_OPTIONS& GetD3D12Options() const;
    BOOL IsUMA() const;
    BOOL IsCacheCoherentUMA() const;
    BOOL IsGPUUploadHeapSupported() const;
    BOOL IsTightAlignmentSupported() const;
    UINT64 GetMemoryCapacity(UINT memorySegmentGroup) const;
    HRESULT CreateResource(
        const ALLOCATION_DESC* pAllocDesc,
        const D3D12_RESOURCE_DESC* pResourceDesc,
        D3D12_RESOURCE_STATES InitialResourceState,
        const D3D12_CLEAR_VALUE *pOptimizedClearValue,
        Allocation** ppAllocation,
        REFIID riidResource,
        void** ppvResource);
#ifdef __ID3D12Device8_INTERFACE_DEFINED__
    HRESULT CreateResource2(
        const ALLOCATION_DESC* pAllocDesc,
        const D3D12_RESOURCE_DESC1* pResourceDesc,
        D3D12_RESOURCE_STATES InitialResourceState,
        const D3D12_CLEAR_VALUE *pOptimizedClearValue,
        Allocation** ppAllocation,
        REFIID riidResource,
        void** ppvResource);
#endif 
#ifdef __ID3D12Device10_INTERFACE_DEFINED__
    HRESULT CreateResource3(const ALLOCATION_DESC* pAllocDesc,
        const D3D12_RESOURCE_DESC1* pResourceDesc,
        D3D12_BARRIER_LAYOUT InitialLayout,
        const D3D12_CLEAR_VALUE* pOptimizedClearValue,
        UINT32 NumCastableFormats,
        const DXGI_FORMAT* pCastableFormats,
        Allocation** ppAllocation,
        REFIID riidResource,
        void** ppvResource);
#endif  
    HRESULT AllocateMemory(
        const ALLOCATION_DESC* pAllocDesc,
        const D3D12_RESOURCE_ALLOCATION_INFO* pAllocInfo,
        Allocation** ppAllocation);
    HRESULT CreateAliasingResource(
        Allocation* pAllocation,
        UINT64 AllocationLocalOffset,
        const D3D12_RESOURCE_DESC* pResourceDesc,
        D3D12_RESOURCE_STATES InitialResourceState,
        const D3D12_CLEAR_VALUE *pOptimizedClearValue,
        REFIID riidResource,
        void** ppvResource);
#ifdef __ID3D12Device8_INTERFACE_DEFINED__
    HRESULT CreateAliasingResource1(Allocation* pAllocation,
        UINT64 AllocationLocalOffset,
        const D3D12_RESOURCE_DESC1* pResourceDesc,
        D3D12_RESOURCE_STATES InitialResourceState,
        const D3D12_CLEAR_VALUE* pOptimizedClearValue,
        REFIID riidResource,
        void** ppvResource);
#endif 
#ifdef __ID3D12Device10_INTERFACE_DEFINED__
    HRESULT CreateAliasingResource2(Allocation* pAllocation,
        UINT64 AllocationLocalOffset,
        const D3D12_RESOURCE_DESC1* pResourceDesc,
        D3D12_BARRIER_LAYOUT InitialLayout,
        const D3D12_CLEAR_VALUE* pOptimizedClearValue,
        UINT32 NumCastableFormats,
        const DXGI_FORMAT* pCastableFormats,
        REFIID riidResource,
        void** ppvResource);
#endif  
    HRESULT CreatePool(
        const POOL_DESC* pPoolDesc,
        Pool** ppPool);
    void SetCurrentFrameIndex(UINT frameIndex);
    void GetBudget(Budget* pLocalBudget, Budget* pNonLocalBudget);
    void CalculateStatistics(TotalStatistics* pStats);
    void BuildStatsString(WCHAR** ppStatsString, BOOL DetailedMap) const;
    void FreeStatsString(WCHAR* pStatsString) const;
    void BeginDefragmentation(const DEFRAGMENTATION_DESC* pDesc, DefragmentationContext** ppContext);
protected:
    void ReleaseThis() override;
private:
    friend D3D12MA_API HRESULT CreateAllocator(const ALLOCATOR_DESC*, Allocator**);
    template<typename T> friend void D3D12MA_DELETE(const ALLOCATION_CALLBACKS&, T*);
    friend class DefragmentationContext;
    friend class Pool;
    Allocator(const ALLOCATION_CALLBACKS& allocationCallbacks, const ALLOCATOR_DESC& desc);
    ~Allocator();
    AllocatorPimpl* m_Pimpl;
    D3D12MA_CLASS_NO_COPY(Allocator)
};
enum VIRTUAL_BLOCK_FLAGS
{
    VIRTUAL_BLOCK_FLAG_NONE = 0,
    VIRTUAL_BLOCK_FLAG_ALGORITHM_LINEAR = POOL_FLAG_ALGORITHM_LINEAR,
    VIRTUAL_BLOCK_FLAG_ALGORITHM_MASK = POOL_FLAG_ALGORITHM_MASK
};
struct VIRTUAL_BLOCK_DESC
{
    VIRTUAL_BLOCK_FLAGS Flags;
    UINT64 Size;
    const ALLOCATION_CALLBACKS* pAllocationCallbacks;
};
enum VIRTUAL_ALLOCATION_FLAGS
{
    VIRTUAL_ALLOCATION_FLAG_NONE = 0,
    VIRTUAL_ALLOCATION_FLAG_UPPER_ADDRESS = ALLOCATION_FLAG_UPPER_ADDRESS,
    VIRTUAL_ALLOCATION_FLAG_STRATEGY_MIN_MEMORY = ALLOCATION_FLAG_STRATEGY_MIN_MEMORY,
    VIRTUAL_ALLOCATION_FLAG_STRATEGY_MIN_TIME = ALLOCATION_FLAG_STRATEGY_MIN_TIME,
    VIRTUAL_ALLOCATION_FLAG_STRATEGY_MIN_OFFSET = ALLOCATION_FLAG_STRATEGY_MIN_OFFSET,
    VIRTUAL_ALLOCATION_FLAG_STRATEGY_MASK = ALLOCATION_FLAG_STRATEGY_MASK,
};
struct VIRTUAL_ALLOCATION_DESC
{
    VIRTUAL_ALLOCATION_FLAGS Flags;
    UINT64 Size;
    UINT64 Alignment;
    void* pPrivateData;
};
struct VIRTUAL_ALLOCATION_INFO
{
    UINT64 Offset;
    UINT64 Size;
    void* pPrivateData;
};
class D3D12MA_API VirtualBlock : public IUnknownImpl
{
public:
    BOOL IsEmpty() const;
    void GetAllocationInfo(VirtualAllocation allocation, VIRTUAL_ALLOCATION_INFO* pInfo) const;
    HRESULT Allocate(const VIRTUAL_ALLOCATION_DESC* pDesc, VirtualAllocation* pAllocation, UINT64* pOffset);
    void FreeAllocation(VirtualAllocation allocation);
    void Clear();
    void SetAllocationPrivateData(VirtualAllocation allocation, void* pPrivateData);
    void GetStatistics(Statistics* pStats) const;
    void CalculateStatistics(DetailedStatistics* pStats) const;
    void BuildStatsString(WCHAR** ppStatsString) const;
    void FreeStatsString(WCHAR* pStatsString) const;
protected:
    void ReleaseThis() override;
private:
    friend D3D12MA_API HRESULT CreateVirtualBlock(const VIRTUAL_BLOCK_DESC*, VirtualBlock**);
    template<typename T> friend void D3D12MA_DELETE(const ALLOCATION_CALLBACKS&, T*);
    VirtualBlockPimpl* m_Pimpl;
    VirtualBlock(const ALLOCATION_CALLBACKS& allocationCallbacks, const VIRTUAL_BLOCK_DESC& desc);
    ~VirtualBlock();
    D3D12MA_CLASS_NO_COPY(VirtualBlock)
};
D3D12MA_API HRESULT CreateAllocator(const ALLOCATOR_DESC* pDesc, Allocator** ppAllocator);
D3D12MA_API HRESULT CreateVirtualBlock(const VIRTUAL_BLOCK_DESC* pDesc, VirtualBlock** ppVirtualBlock);
#ifndef D3D12MA_NO_HELPERS
struct CALLOCATION_DESC : public ALLOCATION_DESC
{
    CALLOCATION_DESC() = default;
    explicit CALLOCATION_DESC(const ALLOCATION_DESC& o) noexcept
        : ALLOCATION_DESC(o)
    {
    }
    explicit CALLOCATION_DESC(Pool* customPool,
        ALLOCATION_FLAGS flags = ALLOCATION_FLAG_NONE,
        void* privateData = NULL) noexcept
    {
        Flags = flags;
        HeapType = (D3D12_HEAP_TYPE)0;
        ExtraHeapFlags = D3D12_HEAP_FLAG_NONE;
        CustomPool = customPool;
        pPrivateData = privateData;
    }
    explicit CALLOCATION_DESC(D3D12_HEAP_TYPE heapType,
        ALLOCATION_FLAGS flags = ALLOCATION_FLAG_NONE,
        void* privateData = NULL,
        D3D12_HEAP_FLAGS extraHeapFlags = D3D12MA_RECOMMENDED_HEAP_FLAGS) noexcept
    {
        Flags = flags;
        HeapType = heapType;
        ExtraHeapFlags = extraHeapFlags;
        CustomPool = NULL;
        pPrivateData = privateData;
    }
};
struct CPOOL_DESC : public POOL_DESC
{
    CPOOL_DESC() = default;
    explicit CPOOL_DESC(const POOL_DESC& o) noexcept
        : POOL_DESC(o)
    {
    }
    explicit CPOOL_DESC(D3D12_HEAP_TYPE heapType,
        D3D12_HEAP_FLAGS heapFlags,
        POOL_FLAGS flags = D3D12MA_RECOMMENDED_POOL_FLAGS,
        UINT64 blockSize = 0,
        UINT minBlockCount = 0,
        UINT maxBlockCount = UINT_MAX,
        D3D12_RESIDENCY_PRIORITY residencyPriority = D3D12_RESIDENCY_PRIORITY_NORMAL) noexcept
    {
        Flags = flags;
        HeapProperties = {};
        HeapProperties.Type = heapType;
        HeapFlags = heapFlags;
        BlockSize = blockSize;
        MinBlockCount = minBlockCount;
        MaxBlockCount = maxBlockCount;
        MinAllocationAlignment = 0;
        pProtectedSession = NULL;
        ResidencyPriority = residencyPriority;
    }
    explicit CPOOL_DESC(const D3D12_HEAP_PROPERTIES heapProperties,
        D3D12_HEAP_FLAGS heapFlags,
        POOL_FLAGS flags = D3D12MA_RECOMMENDED_POOL_FLAGS,
        UINT64 blockSize = 0,
        UINT minBlockCount = 0,
        UINT maxBlockCount = UINT_MAX,
        D3D12_RESIDENCY_PRIORITY residencyPriority = D3D12_RESIDENCY_PRIORITY_NORMAL) noexcept
    {
        Flags = flags;
        HeapProperties = heapProperties;
        HeapFlags = heapFlags;
        BlockSize = blockSize;
        MinBlockCount = minBlockCount;
        MaxBlockCount = maxBlockCount;
        MinAllocationAlignment = 0;
        pProtectedSession = NULL;
        ResidencyPriority = residencyPriority;
    }
};
struct CVIRTUAL_BLOCK_DESC : public VIRTUAL_BLOCK_DESC
{
    CVIRTUAL_BLOCK_DESC() = default;
    explicit CVIRTUAL_BLOCK_DESC(const VIRTUAL_BLOCK_DESC& o) noexcept
        : VIRTUAL_BLOCK_DESC(o)
    {
    }
    explicit CVIRTUAL_BLOCK_DESC(UINT64 size,
        VIRTUAL_BLOCK_FLAGS flags = VIRTUAL_BLOCK_FLAG_NONE,
        const ALLOCATION_CALLBACKS* allocationCallbacks = NULL) noexcept
    {
        Flags = flags;
        Size = size;
        pAllocationCallbacks = allocationCallbacks;
    }
};
struct CVIRTUAL_ALLOCATION_DESC : public VIRTUAL_ALLOCATION_DESC
{
    CVIRTUAL_ALLOCATION_DESC() = default;
    explicit CVIRTUAL_ALLOCATION_DESC(const VIRTUAL_ALLOCATION_DESC& o) noexcept
        : VIRTUAL_ALLOCATION_DESC(o)
    {
    }
    explicit CVIRTUAL_ALLOCATION_DESC(UINT64 size, UINT64 alignment,
        VIRTUAL_ALLOCATION_FLAGS flags = VIRTUAL_ALLOCATION_FLAG_NONE,
        void* privateData = NULL) noexcept
    {
        Flags = flags;
        Size = size;
        Alignment = alignment;
        pPrivateData = privateData;
    }
};
#endif 
} 
DEFINE_ENUM_FLAG_OPERATORS(D3D12MA::ALLOCATION_FLAGS);
DEFINE_ENUM_FLAG_OPERATORS(D3D12MA::DEFRAGMENTATION_FLAGS);
DEFINE_ENUM_FLAG_OPERATORS(D3D12MA::ALLOCATOR_FLAGS);
DEFINE_ENUM_FLAG_OPERATORS(D3D12MA::POOL_FLAGS);
DEFINE_ENUM_FLAG_OPERATORS(D3D12MA::VIRTUAL_BLOCK_FLAGS);
DEFINE_ENUM_FLAG_OPERATORS(D3D12MA::VIRTUAL_ALLOCATION_FLAGS);
