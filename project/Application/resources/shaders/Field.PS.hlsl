//======================================================
// FieldCylinder.PS.hlsl
// 砂色 + 多層ノイズ + 外周フェード + 距離リング
//======================================================

Texture2D gTexture : register(t0); // 今は色には使ってない
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

// 疑似 2D ノイズ
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

    float3 sandTintInner = float3(0.78, 0.74, 0.68); // 内側：明るめ
    float3 sandTintOuter = float3(0.70, 0.66, 0.60); // 外側：少し暗め
    float3 nightTint = float3(0.12, 0.16, 0.25); // 夜の青

    float tRingColor = saturate(distUV / 0.5); // 0〜0.5あたりを正規化
    float3 sandTint = lerp(sandTintInner, sandTintOuter, tRingColor);

    const float nightMix = 0.45; // 夜の混ぜ具合(0〜1)
    float3 baseColor = lerp(sandTint, nightTint, nightMix);

    //--------------------------------------------------
    // ② 多層ノイズで砂目を作る
    //--------------------------------------------------
    float2 uvFine = input.uv * 80.0;
    float nFine = Hash21(uvFine);

    float2 uvCoarse = input.uv * 12.0;
    float nCoarse = Hash21(uvCoarse);

    float2 uvSpot = input.uv * 5.0;
    float nSpot = Hash21(uvSpot);

    float gFine = (nFine * 2.0 - 1.0);
    float gCoarse = (nCoarse * 2.0 - 1.0);

    float fineStrength = 0.06;
    float coarseStrength = 0.10;

    float spotMask = step(0.82, nSpot); // たまにだけ 1 になる
    float spotStrength = -0.15 * spotMask; // たまに暗い粒

    float grainFactor =
        1.0
        + gFine * fineStrength
        + gCoarse * coarseStrength
        + spotStrength;

    float3 noisyColor = baseColor * grainFactor;

    //--------------------------------------------------
    // ③ 法線で“傾斜部分を少し暗く”
    //--------------------------------------------------
    float vertical = saturate(input.normal.y);
    float slopeDark = lerp(0.85, 1.0, vertical);
    noisyColor *= slopeDark;

    //--------------------------------------------------
    // ④ 距離リング（同心円の目安）を追加
    //--------------------------------------------------
    // distUV は 0〜約0.5 くらい。ここにリングを周期的に入れる
    const float ringCount = 8.0; // ★ リングの本数（増やすと細かく）
    const float ringWidth = 0.06; // ★ 線の太さ
    const float ringDarkMul = 0.75; // ★ 線部分でどれだけ暗くするか

    float ringPhase = frac(distUV * ringCount); // 0〜1 を ringCount 回くり返し
    // 0 に近いところだけ線にする（なめらかに）
    float ringLine = SmoothStep(0.0, ringWidth, ringWidth - ringPhase);

    // 線部分だけ少し暗くする
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
