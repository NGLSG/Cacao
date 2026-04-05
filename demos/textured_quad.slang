// 顶点着色器输入：从 CPU 端的顶点缓冲区传入
struct VSInput
{
    float2 position : POSITION;   // 顶点位置（NDC 坐标，-1 到 1）
    float2 texcoord : TEXCOORD;   // 纹理坐标（0 到 1）
};

// 顶点着色器输出 / 片段着色器输入
struct VSOutput
{
    float4 position : SV_Position; // 裁剪空间位置（GPU 光栅化用）
    float2 texcoord : TEXCOORD;    // 插值后的纹理坐标，传给片段着色器
};

// 绑定的纹理资源（register(t0) 对应 DescriptorSet 中 binding=0 的 SampledImage）
Texture2D<float4> gTexture : register(t0);

// 绑定的采样器（register(s1) 对应 DescriptorSet 中 binding=1 的 Sampler）
SamplerState gSampler : register(s1);

// 顶点着色器：将 2D 位置扩展为 4D 裁剪坐标，直传纹理坐标
[shader("vertex")]
VSOutput mainVS(VSInput input)
{
    VSOutput output;
    output.position = float4(input.position, 0.0, 1.0); // z=0, w=1
    output.texcoord = input.texcoord;
    return output;
}

// 片段着色器：用插值后的纹理坐标采样纹理，输出像素颜色
[shader("fragment")]
float4 mainPS(VSOutput input) : SV_Target
{
    return gTexture.Sample(gSampler, input.texcoord);
}
