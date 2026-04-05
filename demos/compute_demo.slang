// Mandelbrot 分形 Compute Shader (Slang)
// 每个线程计算一个像素的 Mandelbrot 迭代，写入 Storage Image

// CPU 传入的参数结构体（与 C++ 端 MandelbrotParams 布局一致）
struct Params
{
    uint width;      // 输出图像宽度（像素）
    uint height;     // 输出图像高度（像素）
    float centerX;   // 分形视图中心 X（复平面实部）
    float centerY;   // 分形视图中心 Y（复平面虚部）
    float zoom;      // 缩放级别（越大越放大）
    uint maxIter;    // 最大迭代次数（决定精度和颜色深度）
    float _pad0;
    float _pad1;
};

// binding 0: 可读写的 Storage Image（compute shader 写入结果）
[[vk::binding(0, 0)]] RWTexture2D<float4> outputImage : register(u0, space0);
// binding 1: 常量缓冲区，存放分形参数
[[vk::binding(1, 0)]] ConstantBuffer<Params> params : register(b1, space0);

// 基于余弦的调色板函数（IQ palette）
// 将 [0,1] 的迭代比例映射为平滑的 RGB 渐变色
float3 palette(float t)
{
    float3 a = float3(0.5, 0.5, 0.5);
    float3 b = float3(0.5, 0.5, 0.5);
    float3 c = float3(1.0, 1.0, 1.0);
    float3 d = float3(0.00, 0.33, 0.67);
    return a + b * cos(6.28318 * (c * t + d));
}

// Compute Shader 入口：16x16 线程组
// 每个线程对应一个输出像素
[shader("compute")]
[numthreads(16, 16, 1)]
void CSMain(uint3 tid : SV_DispatchThreadID)
{
    // 边界检查：超出图像范围的线程直接返回
    if (tid.x >= params.width || tid.y >= params.height)
        return;

    // 将像素坐标映射到复平面坐标
    float aspect = float(params.width) / float(params.height);
    float x0 = params.centerX + (float(tid.x) / float(params.width) - 0.5) * 3.5 / params.zoom * aspect;
    float y0 = params.centerY + (float(tid.y) / float(params.height) - 0.5) * 3.5 / params.zoom;

    // Mandelbrot 迭代：z = z^2 + c，判断是否发散（|z| > 2）
    float x = 0.0, y = 0.0;
    uint iter = 0;
    for (; iter < params.maxIter; iter++)
    {
        float x2 = x * x, y2 = y * y;
        if (x2 + y2 > 4.0) break;
        y = 2.0 * x * y + y0;
        x = x2 - y2 + x0;
    }

    // 着色：集合内部为黑色，外部按迭代次数映射调色板
    float3 color;
    if (iter >= params.maxIter)
        color = float3(0, 0, 0);
    else
    {
        float t = float(iter) / float(params.maxIter);
        color = palette(t);
    }

    // 写入 Storage Image（RWTexture2D）
    outputImage[tid.xy] = float4(color, 1.0);
}
