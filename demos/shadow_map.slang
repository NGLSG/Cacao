// ============================================================
// Shadow Map 示例 (Slang)
//
// 本着色器实现两遍渲染的阴影映射算法：
//   第一遍（Shadow Pass）：从光源视角渲染场景，仅写入深度
//   第二遍（Main Pass）：从相机视角渲染场景，采样深度图判断阴影
// ============================================================

// 场景参数 UBO，CPU 每帧更新
struct SceneParams
{
    float4x4 cameraVP;  // 相机 View-Projection 矩阵
    float4x4 lightVP;   // 光源 View-Projection 矩阵（用于生成和查询深度图）
    float3 lightDir;     // 方向光方向（归一化，指向光照来源的反方向为照亮方向）
    float _pad0;         // 对齐填充
};

// 绑定资源：UBO + 阴影深度图 + 采样器
[[vk::binding(0, 0)]] ConstantBuffer<SceneParams> scene : register(b0, space0);
[[vk::binding(1, 0)]] Texture2D<float> shadowTex : register(t1, space0);  // 第一遍渲染产生的深度图
[[vk::binding(2, 0)]] SamplerState shadowSamp : register(s2, space0);      // 最近邻采样，边界返回白色（=不遮挡）

// ========================
// 第一遍：阴影深度通道
// ========================
// 只需要顶点着色器，将场景变换到光源的裁剪空间。
// GPU 自动将 SV_Position.z 写入深度缓冲（即"阴影深度图"）。
// 无需片段着色器——不输出颜色，只写深度。

struct ShadowVSOut
{
    float4 position : SV_Position;
};

[shader("vertex")]
ShadowVSOut ShadowVS(float3 pos : POSITION)
{
    ShadowVSOut o;
    o.position = mul(float4(pos, 1.0), scene.lightVP);
    return o;
}

// ========================
// 第二遍：主渲染通道
// ========================

struct MainVSOut
{
    float4 position : SV_Position;  // 相机裁剪空间位置
    float3 worldPos : TEXCOORD0;    // 世界空间位置
    float3 normal : TEXCOORD1;      // 法线（用于光照计算）
    float3 color : TEXCOORD2;       // 顶点颜色
    float4 lightClip : TEXCOORD3;   // 光源裁剪空间位置（用于查询深度图）
};

[shader("vertex")]
MainVSOut MainVS(float3 pos : POSITION, float3 norm : NORMAL, float3 col : COLOR)
{
    MainVSOut o;
    o.worldPos = pos;
    o.normal = norm;
    o.color = col;
    o.position = mul(float4(pos, 1.0), scene.cameraVP);   // 变换到相机裁剪空间 → 屏幕
    o.lightClip = mul(float4(pos, 1.0), scene.lightVP);   // 同时变换到光源裁剪空间 → 查阴影
    return o;
}

static const float SM_RES = 2048.0;  // 阴影深度图分辨率

[shader("fragment")]
float4 MainPS(MainVSOut input) : SV_Target
{
    float3 N = normalize(input.normal);
    float3 L = normalize(-scene.lightDir);  // 光照方向（指向光源）

    // --- 阴影判定 ---
    // 1. 透视除法：光源裁剪空间 → 光源 NDC（-1~1）
    float3 ndc = input.lightClip.xyz / input.lightClip.w;

    // 2. NDC → 深度图 UV（0~1），Y 翻转适配 Vulkan 坐标系
    float2 uv = ndc.xy * 0.5 + 0.5;
    uv.y = 1.0 - uv.y;

    // 3. 当前片元在光源视角的深度（映射到 0~1）
    float depth = ndc.z * 0.5 + 0.5;

    // 4. PCF（Percentage-Closer Filtering）3×3 软阴影
    //    采样周围 9 个纹素的深度值，与当前深度比较：
    //    如果当前深度 > 深度图值+偏移，说明被其他物体挡住 → 在阴影中
    //    多次采样取平均产生柔和的阴影边缘
    float shadow = 0.0;
    float2 texel = float2(1.0 / SM_RES, 1.0 / SM_RES);
    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            float d = shadowTex.SampleLevel(shadowSamp, uv + float2(x, y) * texel, 0);
            shadow += (depth - 0.003 > d) ? 0.0 : 1.0;  // 0.003 偏移防止自阴影伪影
        }
    }
    shadow /= 9.0;

    // 超出深度图范围的区域视为完全照亮
    if (uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1)
        shadow = 1.0;

    // --- 光照计算（Lambert 漫反射） ---
    float NdotL = max(dot(N, L), 0.0);
    float3 ambient = input.color * 0.15;             // 环境光（始终可见）
    float3 diffuse = input.color * NdotL * shadow;   // 漫反射（受阴影遮挡）
    float3 result = ambient + diffuse;

    // Gamma 校正（线性空间 → sRGB）
    result = pow(max(result, float3(0, 0, 0)), 1.0 / 2.2);

    return float4(result, 1.0);
}
