// Shadow Map Demo

struct SceneParams
{
    float4x4 cameraVP;
    float4x4 lightVP;
    float3   lightPos;
    float    _pad0;
    float3   cameraPos;
    float    _pad1;
};

[[vk::binding(0, 0)]]
cbuffer SceneBuffer : register(b0) { SceneParams scene; };

[[vk::binding(1, 0)]]
Texture2D shadowTex : register(t1);

[[vk::binding(2, 0)]]
SamplerState shadowSamp : register(s2);

// ─── Shadow pass ─────────────────────────────────────────
struct ShadowVSIn  { float3 pos : POSITION; };
struct ShadowVSOut { float4 position : SV_Position; };

[shader("vertex")]
ShadowVSOut shadowVS(ShadowVSIn input)
{
    ShadowVSOut o;
    o.position = mul(float4(input.pos, 1.0), scene.lightVP);
    return o;
}

[shader("fragment")]
float4 shadowPS(ShadowVSOut input) : SV_Target
{
    return float4(input.position.z, 0, 0, 1);
}

// ─── Main pass ───────────────────────────────────────────
struct MainVSIn
{
    float3 pos    : POSITION;
    float3 normal : NORMAL;
    float3 color  : COLOR;
};

struct MainVSOut
{
    float4 position  : SV_Position;
    float3 worldPos  : TEXCOORD0;
    float3 normal    : TEXCOORD1;
    float3 color     : TEXCOORD2;
    float4 lightClip : TEXCOORD3;
};

[shader("vertex")]
MainVSOut mainVS(MainVSIn input)
{
    MainVSOut o;
    o.worldPos  = input.pos;
    o.normal    = input.normal;
    o.color     = input.color;
    o.position  = mul(float4(input.pos, 1.0), scene.cameraVP);
    o.lightClip = mul(float4(input.pos, 1.0), scene.lightVP);
    return o;
}

[shader("fragment")]
float4 mainPS(MainVSOut input) : SV_Target
{
    float3 N = normalize(input.normal);
    float3 toLight = scene.lightPos - input.worldPos;
    float  dist = length(toLight);
    float3 L = toLight / dist;

    // Shadow
    float shadow = 1.0;
    float3 ndc = input.lightClip.xyz / input.lightClip.w;
    if (ndc.z >= 0.0 && ndc.z <= 1.0)
    {
        float2 uv = ndc.xy * 0.5 + 0.5;
        if (uv.x > 0.001 && uv.x < 0.999 && uv.y > 0.001 && uv.y < 0.999)
        {
            float depth = ndc.z;
            float d = shadowTex.SampleLevel(shadowSamp, uv, 0).r;
            shadow = (depth > d + 0.005) ? 0.15 : 1.0;
        }
    }

    float atten = 1.0 / (1.0 + 0.07 * dist + 0.017 * dist * dist);
    float NdotL = max(dot(N, L), 0.0);

    float3 V = normalize(scene.cameraPos - input.worldPos);
    float3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 32.0) * 0.3;

    float3 ambient = input.color * 0.06;
    float3 diffuse = input.color * NdotL * shadow * atten;
    float3 specular = float3(spec, spec, spec) * shadow * atten;

    float3 result = ambient + diffuse + specular;
    result = pow(max(result, 0.0), 1.0 / 2.2);
    return float4(result, 1.0);
}
