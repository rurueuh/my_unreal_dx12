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

    float3 uKs;
    float uOpacity;

    float3 uKe;
    float _pad1;
};

Texture2D uTexture : register(t0);
Texture2D uShadowMap : register(t1);
Texture2D uNormalMap : register(t2);

SamplerState uSampler : register(s0);
SamplerState uShadowSampler : register(s1);

static const float3 kLightColor = float3(1.0, 0.98, 0.90);
static const float3 kAmbient = float3(0.25, 0.25, 0.25);
static const float SHADOW_MAP_SIZE = 4096.0f;

static const int POISSON_MAX = 32;
static const int POISSON_SAMPLES = 16;

static float2 poissonDisk[POISSON_MAX] =
{
    float2(0.0625f, 0.1875f),
    float2(-0.1875f, 0.0625f),
    float2(0.1875f, -0.0625f),
    float2(-0.0625f, -0.1875f),

    float2(0.3750f, 0.1250f),
    float2(-0.3750f, 0.1250f),
    float2(0.1250f, -0.3750f),
    float2(-0.1250f, -0.3750f),

    float2(0.5625f, 0.3125f),
    float2(-0.5625f, 0.3125f),
    float2(0.3125f, -0.5625f),
    float2(-0.3125f, -0.5625f),

    float2(0.7500f, 0.5000f),
    float2(-0.7500f, 0.5000f),
    float2(0.5000f, -0.7500f),
    float2(-0.5000f, -0.7500f),

    float2(0.1875f, 0.5625f),
    float2(-0.1875f, 0.5625f),
    float2(0.5625f, 0.1875f),
    float2(-0.5625f, 0.1875f),

    float2(0.3125f, 0.7500f),
    float2(-0.3125f, 0.7500f),
    float2(0.7500f, 0.3125f),
    float2(-0.7500f, 0.3125f),

    float2(0.4375f, 0.9375f),
    float2(-0.4375f, 0.9375f),
    float2(0.9375f, 0.4375f),
    float2(-0.9375f, 0.4375f),

    float2(0.2500f, 0.8750f),
    float2(-0.2500f, 0.8750f),
    float2(0.8750f, 0.2500f),
    float2(-0.8750f, 0.2500f)
};

struct VSOut
{
    float4 pos : SV_Position;
    float3 worldPos : TEXCOORD0;
    float3 nrm : NORMAL1;
    float3 col : COLOR1;
    float2 uv : TEXCOORD2;
    float4 shadowPos : TEXCOORD3;
    float3 tangent : TEXCOORD4;
    float3 bitangent : TEXCOORD5;
};

float Hash12(float2 p)
{
    float3 p3 = frac(float3(p.x, p.y, p.x) * 0.1031f);
    p3 += dot(p3, p3.yzx + 33.33f);
    return frac((p3.x + p3.y) * p3.z);
}

float ComputeShadow(float4 shadowPos, float3 normal, float4 screenPos)
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

    float baseBias = 0.0006f;
    float slopeBias = 0.0045f * (1.0f - NdotL);
    float bias = baseBias + slopeBias;

    float2 texel = float2(1.0 / SHADOW_MAP_SIZE, 1.0 / SHADOW_MAP_SIZE);

    float rnd = Hash12(screenPos.xy * 0.5f);
    float angle = rnd * 6.2831853f;
    float s = sin(angle);
    float c = cos(angle);
    float2x2 rot = float2x2(c, -s, s, c);

    float radius = 3.0f;

    float acc = 0.0f;
    int count = 0;

    [unroll]
    for (int i = 0; i < POISSON_SAMPLES; ++i)
    {
        float2 o = mul(poissonDisk[i], rot) * radius * texel;
        float2 uv = baseUV + o;

        if (uv.x < 0.0 || uv.x > 1.0 ||
            uv.y < 0.0 || uv.y > 1.0)
        {
            continue;
        }

        float sd = uShadowMap.Sample(uShadowSampler, uv).r;
        float lit = step(depth - bias, sd);
        acc += lit;
        count++;
    }

    if (count == 0)
        return 1.0;

    return acc / count;
}

float4 main(VSOut i) : SV_Target
{
    float3 N = normalize(i.nrm);

    float tLen2 = dot(i.tangent, i.tangent);
    float bLen2 = dot(i.bitangent, i.bitangent);

    bool useNormalMap = (tLen2 > 1e-6f) && (bLen2 > 1e-6f);

    if (useNormalMap)
    {
        float3 Ng = normalize(i.nrm);
        float3 T = normalize(i.tangent);
        float3 B = normalize(i.bitangent);
        T = normalize(T - Ng * dot(T, Ng));
        B = normalize(B - Ng * dot(B, Ng));
        float3x3 TBN = float3x3(T, B, Ng);
        float3 nTex = uNormalMap.Sample(uSampler, i.uv).xyz;
        nTex = nTex * 2.0f - 1.0f;
        N = normalize(mul(nTex, TBN));
    }

    float3 L = normalize(uLightDir);
    float3 V = normalize(uCameraPos - i.worldPos);
    float3 R = reflect(-L, N);

    float NdotL = saturate(dot(N, L));
    float3 diffuse = kLightColor * NdotL;

    float spec = 0.0f;
    if (NdotL > 0.0f)
        spec = pow(saturate(dot(R, V)), uShininess);

    float3 specular = kLightColor * spec * uKs;

    float shadow = ComputeShadow(i.shadowPos, N, i.pos);

    float4 texSample = uTexture.Sample(uSampler, i.uv);
    float3 albedo = texSample.rgb * i.col;

    float3 lighting = kAmbient + shadow * (diffuse + specular);
    float3 color = albedo * lighting + uKe;

    float alpha = texSample.a * uOpacity;

    return float4(color, alpha);
}
