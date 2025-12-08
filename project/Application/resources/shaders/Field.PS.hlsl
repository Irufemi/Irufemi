//======================================================
// Field.PS.hlsl
// 円柱フィールド用：砂色 + 外周フェード + 擬似砂ノイズ（テクスチャ色は無視）
//======================================================

Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PSInput
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
};

float SmoothStep(float edge0, float edge1, float x)
{
    float t = saturate((x - edge0) / (edge1 - edge0));
    return t * t * (3.0 - 2.0 * t);
}

float Hash21(float2 p)
{
    p = frac(p * float2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return frac(p.x * p.y);
}

float4 main(PSInput input) : SV_TARGET
{
    //--------------------------------------------------
    // ① ベース砂色（テクスチャの色は使わない）
    //--------------------------------------------------
    float3 sandTint = float3(0.85, 0.80, 0.70); // 砂
    float3 nightTint = float3(0.25, 0.30, 0.45); // 夜

    const float nightMix = 0.3; 
    float3 baseColor = lerp(sandTint, nightTint, nightMix);

    //--------------------------------------------------
    // ② 擬似砂ノイズ（明るさをちょっとだけ揺らす）
    //--------------------------------------------------
    float2 noiseUV = input.uv * 40.0;
    float n = Hash21(noiseUV); // 0〜1

    float grain = (n * 2.0 - 1.0) * 0.12; // 砂目の強さ
    float3 noisyColor = baseColor * (1.0 + grain);

    //--------------------------------------------------
    // ③ 外周フェード（中心明るく・外側暗く）
    //--------------------------------------------------
    float2 centerUV = float2(0.5, 0.5);
    float dist = length(input.uv - centerUV);

    const float fadeStart = 0.35;
    const float fadeEnd = 0.48;
    float k = SmoothStep(fadeStart, fadeEnd, dist);

    const float minFactor = 0.2;
    float fadeMul = lerp(1.0, minFactor, k);

    float3 finalColor = noisyColor * fadeMul;

    return float4(finalColor, 1.0);
}
