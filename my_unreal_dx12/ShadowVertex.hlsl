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
};

struct VSOut
{
    float4 pos : SV_Position;
};

VSOut main(VSIn v)
{
    VSOut o;
    float4 w = mul(float4(v.pos, 1.0), uModel);
    o.pos = mul(w, uLightViewProj);
    return o;
}
