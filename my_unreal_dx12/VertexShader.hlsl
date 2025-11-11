cbuffer Scene : register(b0)
{
    float4x4 uModel;
    float4x4 uViewProj;
    float4x4 uNormalMatrix;
}

struct VSIn
{
    float3 pos : POSITION;
    float3 nrm : NORMAL0;
    float3 col : COLOR0;
    float2 uv : TEXCOORD0;
};
struct VSOut
{
    float4 pos : SV_Position;
    float3 nrm : NORMAL0;
    float3 col : COLOR0;
    float2 uv : TEXCOORD0;
};

VSOut main(VSIn v)
{
    VSOut o;
    float4 wpos = mul(float4(v.pos, 1.0), uModel);
    o.pos = mul(wpos, uViewProj);

    float3x3 Nmat = (float3x3) uNormalMatrix;
    o.nrm = normalize(mul(v.nrm, Nmat));

    o.col = v.col;
    o.uv = v.uv;
    return o;
}