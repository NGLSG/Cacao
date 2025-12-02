// ======================
// Sprite Batch Shader
// Instanced + Batched Rendering
// ======================

struct SpriteInstance
{
    float2 position;    // 精灵位置 (屏幕空间 -1 到 1)
    float2 scale;       // 精灵缩放
    float4 uvRect;      // UV矩形 (u0, v0, u1, v1)
    float4 color;       // 颜色调制
    float rotation;     // 旋转角度 (弧度)
    float _padding[3];  // 对齐到16字节
};

struct VSInput
{
    uint vertexID : SV_VertexID;
    uint instanceID : SV_InstanceID;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};

// 描述符绑定
[[vk::binding(0, 0)]]
StructuredBuffer<SpriteInstance> sprites;

[[vk::binding(1, 0)]]
Texture2D spriteTexture;

[[vk::binding(2, 0)]]
SamplerState spriteSampler;

// 四边形顶点 (两个三角形)
static const float2 quadPositions[6] =
{
    float2(-0.5, -0.5),  // 左下
    float2( 0.5, -0.5),  // 右下
    float2( 0.5,  0.5),  // 右上
    float2(-0.5, -0.5),  // 左下
    float2( 0.5,  0.5),  // 右上
    float2(-0.5,  0.5),  // 左上
};

static const float2 quadUVs[6] =
{
    float2(0.0, 1.0),  // 左下
    float2(1.0, 1.0),  // 右下
    float2(1.0, 0.0),  // 右上
    float2(0.0, 1.0),  // 左下
    float2(1.0, 0.0),  // 右上
    float2(0.0, 0.0),  // 左上
};

[shader("vertex")]
VSOutput mainVS(VSInput input)
{
    VSOutput output;
    
    SpriteInstance sprite = sprites[input.instanceID];
    float2 localPos = quadPositions[input.vertexID];
    
    // 应用旋转
    float cosR = cos(sprite.rotation);
    float sinR = sin(sprite.rotation);
    float2 rotatedPos;
    rotatedPos.x = localPos.x * cosR - localPos.y * sinR;
    rotatedPos.y = localPos.x * sinR + localPos.y * cosR;
    
    // 应用缩放和位置
    float2 worldPos = rotatedPos * sprite.scale + sprite.position;
    
    output.position = float4(worldPos, 0.0, 1.0);
    
    // 计算UV (从uvRect插值)
    float2 baseUV = quadUVs[input.vertexID];
    output.uv = lerp(sprite.uvRect.xy, sprite.uvRect.zw, baseUV);
    
    output.color = sprite.color;
    
    return output;
}

[shader("fragment")]
float4 mainPS(VSOutput input) : SV_Target
{
    float4 texColor = spriteTexture.Sample(spriteSampler, input.uv);
    return texColor * input.color;
}
