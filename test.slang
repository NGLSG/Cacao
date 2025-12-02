// ======================
// Vertex Shader
// ======================
static const float2 positions[3] =
{
    float2( 0.0, -0.5 ),
    float2( 0.5,  0.5 ),
    float2(-0.5,  0.5 )
};

[shader("vertex")]
float4 mainVS(uint vertexID : SV_VertexID) : SV_Position
{
    float2 pos = positions[vertexID];
    return float4(pos, 0.0, 1.0);
}

// ======================
// Pixel / Fragment Shader
// ======================

[shader("fragment")]
float4 mainPS() : SV_Target
{
    return float4(1.0, 1.0, 1.0, 1.0);
}