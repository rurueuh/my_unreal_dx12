cbuffer Scene : register(b0)
{
    float4x4 uModel;
    float4x4 uViewProj;
    float4x4 uNormalMatrix;

    float3 uCameraPos;
    float uShininess;

    float4x4 uLightViewProj;

    float3 uLightDir;
    float _pad0;
};

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
    float3 worldPos : TEXCOORD0;
    float3 nrm : NORMAL1;
    float3 col : COLOR1;
    float2 uv : TEXCOORD2;
    float4 shadowPos : TEXCOORD3;
};

VSOut main(VSIn v)
{
    VSOut o;

    float4 w = mul(float4(v.pos, 1.0), uModel);
    o.worldPos = w.xyz;

    o.pos = mul(w, uViewProj);

    float3x3 nMat = (float3x3) uNormalMatrix;
    o.nrm = normalize(mul(v.nrm, nMat));

    o.col = v.col;
    o.uv = v.uv;

    o.shadowPos = mul(w, uLightViewProj);

    return o;
}
