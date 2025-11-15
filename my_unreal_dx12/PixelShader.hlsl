Texture2D uTexture : register(t0);
SamplerState uSampler : register(s0);

static const float3 kLightDir = normalize(float3(0.0, 1.0, 0.0)); 
static const float3 kLightColor = float3(1.0, 0.98, 0.90);
static const float3 kAmbient = float3(0.2, 0.2, 0.2);
static const float kSpecStrength = 0.5f;

cbuffer Scene : register(b0)
{
    float4x4 uModel;
    float4x4 uViewProj;
    float4x4 uNormalMatrix;

    float3 uCameraPos;
    float uShininess;
};

float4 main(float4 pos : SV_Position,
            float3 nrm : NORMAL0,
            float3 col : COLOR0,
            float2 uv : TEXCOORD0,
            float3 worldPos : TEXCOORD1) : SV_Target
{
    float3 N = normalize(nrm);

    float3 L = normalize(kLightDir);
    float NdotL = saturate(dot(N, L));

    float3 diffuse = kLightColor * NdotL;
    float3 ambient = kAmbient;

    float3 V = normalize(uCameraPos - worldPos);

    float3 R = reflect(-L, N);

    float spec = pow(saturate(dot(R, V)), uShininess);
    float3 specular = kLightColor * spec * kSpecStrength;

    float3 base = uTexture.Sample(uSampler, uv).rgb * col;

    float3 lit = ambient + diffuse + specular;
    return float4(base * lit, 1.0);
}
