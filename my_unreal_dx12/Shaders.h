#pragma once
static const char* kVertexShaderSrc = R"(
cbuffer Scene : register(b0)
{
    float4x4 uModel;
    float4x4 uViewProj;
    float4x4 uNormalMatrix;
}

struct VSIn {
    float3 pos : POSITION;
    float3 nrm : NORMAL0;
    float3 col : COLOR0;
    float2 uv  : TEXCOORD0;
};
struct VSOut {
    float4 pos : SV_Position;
    float3 nrm : NORMAL0;
    float3 col : COLOR0;
    float2 uv  : TEXCOORD0;
};

VSOut main(VSIn v)
{
    VSOut o;
    float4 wpos = mul(float4(v.pos, 1.0), uModel);
    o.pos = mul(wpos, uViewProj);

    float3x3 Nmat = (float3x3)uNormalMatrix;
    o.nrm = normalize(mul(v.nrm, Nmat));

    o.col = v.col;
    o.uv  = v.uv;
    return o;
}
)";

static const char* kPixelShaderSrc = R"(
Texture2D    uTexture : register(t0);
SamplerState uSampler : register(s0);

static const float3 kLightDir   = normalize(float3(0.0, 1.0, 0.0));
static const float3 kLightColor = float3(1.0, 0.98, 0.90);
static const float3 kAmbient    = float3(0.2, 0.2, 0.2);

float4 main(float4 pos:SV_Position, float3 nrm:NORMAL0, float3 col:COLOR0, float2 uv:TEXCOORD0) : SV_Target
{
    float3 N = normalize(nrm);
    float  NdotL = saturate(dot(N, kLightDir));
    float3 lit = kAmbient + kLightColor * NdotL;

    float3 base = uTexture.Sample(uSampler, uv).rgb * col;
    return float4(base * lit, 1.0);
}
)";
