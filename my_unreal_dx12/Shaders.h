#pragma once
static const char* kVertexShaderSrc = R"(
cbuffer Scene : register(b0) { float4x4 uMVP; }

struct VSIn {
    float3 pos : POSITION;
    float3 col : COLOR0;
    float2 uv  : TEXCOORD0;
};
struct VSOut {
    float4 pos : SV_Position;
    float3 col : COLOR0;
    float2 uv  : TEXCOORD0;
};

VSOut main(VSIn v)
{
    VSOut o;
    o.pos = mul(float4(v.pos,1), uMVP);
    o.col = v.col;
    o.uv  = v.uv;
    return o;
}
)";


static const char* kPixelShaderSrc = R"(
Texture2D    uTexture : register(t0);
SamplerState uSampler : register(s0);

float4 main(float4 pos : SV_Position, float3 col : COLOR0, float2 uv : TEXCOORD0) : SV_Target
{
    float2 uvFix = float2(1-uv.x, uv.y);
    float4 texColor = uTexture.Sample(uSampler, uvFix);
    //texColor = float4(1,1,1,1);
    return texColor * float4(col, 1);
}
)";