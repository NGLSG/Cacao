struct SpriteInstance {
    float2 position;
    float2 scale;
    float4 uvRect;
    float4 color;
    float rotation;
    float3 _padding;
};

struct UpdateParams {
    float deltaTime;
    float time;
    uint spriteCount;
    float _pad;
};

[[vk::binding(0, 0)]]
RWStructuredBuffer<SpriteInstance> sprites;

[[vk::binding(1, 0)]]
ConstantBuffer<UpdateParams> params;

[shader("compute")]
[numthreads(256, 1, 1)]
void mainCS(uint3 tid : SV_DispatchThreadID) {
    uint idx = tid.x;
    if (idx >= params.spriteCount) return;

    SpriteInstance s = sprites[idx];
    float phase = float(idx) * 0.001;
    float t = params.time;

    s.position.x += sin(t * 2.0 + phase) * 0.0001;
    s.position.y += cos(t * 2.0 + phase) * 0.0001;

    if (s.position.x > 1.0) s.position.x = -1.0;
    if (s.position.x < -1.0) s.position.x = 1.0;
    if (s.position.y > 1.0) s.position.y = -1.0;
    if (s.position.y < -1.0) s.position.y = 1.0;

    s.rotation += params.deltaTime * 0.5;

    sprites[idx] = s;
}
