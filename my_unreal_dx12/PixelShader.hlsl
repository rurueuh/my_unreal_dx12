Texture2D uTexture : register(t0);
SamplerState uSampler : register(s0);

static const float3 kLightDir = normalize(float3(0.0, 1.0, 0.0));
static const float3 kLightColor = float3(1.0, 0.98, 0.90);
static const float3 kAmbient = float3(0.2, 0.2, 0.2);

float4 main(float4 pos : SV_Position, float3 nrm : NORMAL0, float3 col : COLOR0, float2 uv : TEXCOORD0) : SV_Target
{
    float3 N = normalize(nrm);
    float NdotL = saturate(dot(N, kLightDir));
    float3 lit = kAmbient + kLightColor * NdotL;

    float3 base = uTexture.Sample(uSampler, uv).rgb * col;
    return float4(base * lit, 1.0);
}