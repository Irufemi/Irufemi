// SkyDome.VS.hlsl

cbuffer TransformCB : register(b0)
{
    float4x4 world;
    float4x4 viewProj;
};

struct VSInput
{
    float4 pos : POSITION; // C++ の InputLayout に合わせる
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
};

struct VSOutput
{
    float4 svpos : SV_POSITION;
    float3 normal : NORMAL; // 法線をそのまま PS に渡す
    float2 uv : TEXCOORD0;
};

VSOutput main(VSInput input)
{
    VSOutput o;

    float4 worldPos = mul(input.pos, world);
    o.svpos = mul(worldPos, viewProj);

    // 法線：とりあえずワールド変換（回転だけ乗る想定）
    float3 worldN = mul(float4(input.normal, 0.0f), world).xyz;
    o.normal = normalize(worldN);

    o.uv = input.uv;
    return o;
}
