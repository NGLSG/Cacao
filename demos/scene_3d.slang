// MVP 矩阵数据结构，从 CPU 端的 Uniform Buffer 传入
struct MVPData
{
    float4x4 mvp; // Model-View-Projection 组合矩阵，将顶点从模型空间变换到裁剪空间
};

// 绑定到描述符集 0 的 binding 0，对应 CPU 端创建的 UniformBuffer
[[vk::binding(0, 0)]]
cbuffer MVPBuffer : register(b0)
{
    MVPData mvpData;
};

// 顶点着色器输入：与 C++ 端的 Vertex 结构体布局一致
struct VSInput
{
    float3 position : TEXCOORD0; // 顶点位置（模型空间）
    float3 color    : TEXCOORD1; // 顶点颜色（每面不同颜色实现彩色立方体）
};

// 顶点着色器输出 / 片段着色器输入
struct VSOutput
{
    float4 position : SV_Position; // 裁剪空间位置，GPU 用于光栅化
    float3 color    : COLOR;       // 传递给片段着色器的插值颜色
};

// 顶点着色器：将模型空间顶点通过 MVP 矩阵变换到裁剪空间
[shader("vertex")]
VSOutput mainVS(VSInput input)
{
    VSOutput output;
    // MVP 变换：position * mvp（行主序乘法）
    output.position = mul(float4(input.position, 1.0), mvpData.mvp);
    output.color = input.color; // 直接传递顶点颜色
    return output;
}

// 片段着色器：输出插值后的顶点颜色
[shader("fragment")]
float4 mainPS(VSOutput input) : SV_Target
{
    return float4(input.color, 1.0); // alpha = 1.0 完全不透明
}
