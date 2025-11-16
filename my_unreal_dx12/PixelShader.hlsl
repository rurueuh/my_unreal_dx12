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

Texture2D uTexture : register(t0);
Texture2D uShadowMap : register(t1);

SamplerState uSampler : register(s0);
SamplerState uShadowSampler : register(s1);

static const float3 kLightColor = float3(1.0, 0.98, 0.90);
static const float3 kAmbient = float3(0.25, 0.25, 0.25);
static const float SHADOW_MAP_SIZE = 2048.0f;

struct VSOut
{
    float4 pos : SV_Position;
    float3 worldPos : TEXCOORD0;
    float3 nrm : NORMAL1;
    float3 col : COLOR1;
    float2 uv : TEXCOORD2;
    float4 shadowPos : TEXCOORD3;
};

float ComputeShadow(float4 shadowPos, float3 normal)
{
    float3 proj = shadowPos.xyz / shadowPos.w;
    float2 baseUV = proj.xy * float2(0.5, -0.5) + 0.5;
    float depth = proj.z;

    if (baseUV.x < 0.0 || baseUV.x > 1.0 ||
        baseUV.y < 0.0 || baseUV.y > 1.0 ||
        depth <= 0.0 || depth >= 1.0)
    {
        return 1.0;
    }

    float3 N = normalize(normal);
    float3 L = normalize(uLightDir);
    float NdotL = saturate(dot(N, L));

    float baseBias = 0.0007f;
    float slopeBias = 0.01f * (1.0f - NdotL);
    float bias = baseBias + slopeBias;

    float2 texelSize = 1.0f / SHADOW_MAP_SIZE;
    float acc = 0.0f;

    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            float2 uv = baseUV + float2(x, y) * texelSize;

            if (uv.x < 0.0 || uv.x > 1.0 ||
                uv.y < 0.0 || uv.y > 1.0)
            {
                acc += 1.0f;
                continue;
            }

            float sd = uShadowMap.Sample(uShadowSampler, uv).r;

            float lit = step(depth - bias, sd);
            acc += lit;
        }
    }

    return acc / 9.0f;
}

float4 main(VSOut i) : SV_Target
{
    float3 N = normalize(i.nrm);
    float3 L = normalize(uLightDir);
    float3 V = normalize(uCameraPos - i.worldPos);
    float3 R = reflect(-L, N);

    float NdotL = saturate(dot(N, L));
    float3 diffuse = kLightColor * NdotL;

    float spec = 0.0f;
    if (NdotL > 0.0f)
    {
        spec = pow(saturate(dot(R, V)), uShininess);
    }
    float3 specular = kLightColor * spec;

    float shadow = ComputeShadow(i.shadowPos, N);

    float3 lighting = kAmbient + shadow * (diffuse + specular);

    float3 albedo = uTexture.Sample(uSampler, i.uv).rgb * i.col;

    return float4(albedo * lighting, 1.0);
}
