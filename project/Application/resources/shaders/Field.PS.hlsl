//======================================================
// FieldCylinder.PS.hlsl
// 砂色 + やわらかノイズ + 静止リング + 外周フェード
//======================================================

Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PSInput
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
};

// ※ timeSec はもう使わないので cbuffer なし

float SmoothStep(float edge0, float edge1, float x)
{
    float t = saturate((x - edge0) / (edge1 - edge0));
    return t * t * (3.0 - 2.0 * t);
}

// 擬似 2D ノイズ
float Hash21(float2 p)
{
    p = frac(p * float2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return frac(p.x * p.y);
}

float4 main(PSInput input) : SV_TARGET
{
    //--------------------------------------------------
    // ① ベース砂色（中心・外側で少しだけ色変える）
    //--------------------------------------------------
    float2 centerUV = float2(0.5, 0.5);
    float distUV = length(input.uv - centerUV); // 0=中心

    float3 sandTintInner = float3(0.78, 0.74, 0.68);
    float3 sandTintOuter = float3(0.70, 0.66, 0.60);
    float3 nightTint = float3(0.12, 0.16, 0.25);

    float tRingColor = saturate(distUV / 0.5);
    float3 sandTint = lerp(sandTintInner, sandTintOuter, tRingColor);

    const float nightMix = 0.45;
    float3 baseColor = lerp(sandTint, nightTint, nightMix);

    //--------------------------------------------------
    // ② ゆるめノイズ（チラつき少なめ）
    //--------------------------------------------------
    float2 uvCoarse = input.uv * 6.0; // 大きめのムラだけ
    float nCoarse = Hash21(uvCoarse);

    float gCoarse = (nCoarse * 2.0 - 1.0);
    float coarseStrength = 0.03;

    float noiseFade = saturate(1.0 - distUV * 1.8); // 中央1 → 外0
    noiseFade = noiseFade * noiseFade;

    float grainFactor = 1.0 + gCoarse * coarseStrength * noiseFade;

    float3 noisyColor = baseColor * grainFactor;

    //--------------------------------------------------
    // ③ 法線で“傾斜部分を少し暗く”
    //--------------------------------------------------
    float vertical = saturate(input.normal.y);
    float slopeDark = lerp(0.94, 1.0, vertical);
    noisyColor *= slopeDark;

    //--------------------------------------------------
    // ④ 静止リング（距離の目安用）
    //--------------------------------------------------
    const float ringCount = 8.0; // リング本数
    const float ringWidth = 0.06; // 太さ
    const float ringDarkMul = 0.78; // 暗さ

    // 距離に応じて周期的なパターンを作る
    float ringPhase = frac(distUV * ringCount);
    float ringLine = SmoothStep(0.0, ringWidth, ringWidth - ringPhase);

    float ringFactor = lerp(1.0, ringDarkMul, ringLine);
    noisyColor *= ringFactor;

    //--------------------------------------------------
    // ⑤ 外周フェード（中心明るく・外側暗く）
    //--------------------------------------------------
    const float fadeStart = 0.32;
    const float fadeEnd = 0.48;
    float k = SmoothStep(fadeStart, fadeEnd, distUV);

    const float minFactor = 0.12;
    float fadeMul = lerp(1.0, minFactor, k);

    float3 finalColor = noisyColor * fadeMul;

    return float4(finalColor, 1.0);
}
