// SkyDome.PS.hlsl --- 月が自然に見えるチューニング済み版 ---

Texture2D skyTex : register(t0); // 夜砂漠グラデーション v2
SamplerState skySamp : register(s0);

struct VSOutput
{
    float4 svpos : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

float4 main(VSOutput input) : SV_TARGET
{
    // ============================
    // 1) 夜砂漠グラデーション
    // ============================
    float3 n = normalize(-input.normal); // 内向きなら反転

    // n.y → 0〜1 に補正して擬似的にグラデーションを作る
    float v = saturate(n.y * 0.5f + 0.5f);
    float3 gradCol = skyTex.Sample(skySamp, float2(0.5f, v)).rgb;

    // 上ほど少し暗く
    float t = pow(v, 1.7f);
    float3 baseCol = gradCol * lerp(0.6f, 1.0f, t);

    // ============================
    // 2) 画面座標で月を描く
    // ============================
    // NDC → Screen(0〜1)
    float2 ndc = input.svpos.xy / input.svpos.w;
    float2 scr = ndc * 0.5f + 0.5f;

    // 月の位置（左上寄り）
    float2 moonCenter = float2(12.0f, 1.0f); // ←ここを動かすと位置調整できる
    float moonRadius = 0.12f; // 月の半径
    float soft = 0.02f; // ぼかし幅

    float dist = length(scr - moonCenter);

    // 月の本体
    float moonMask = 1.0f - smoothstep(moonRadius,
                                       moonRadius + soft, dist);

    // ハロー
    float haloR = moonRadius + soft;
    float haloMask = 1.0f - smoothstep(haloR,
                                       haloR + 0.08f, dist);

    // ============================
    // 3) 地平線グロー
    // ============================
    float horizonGlow =
        pow(saturate(1.0f - abs(n.y)), 4.0f) * 0.08f;

    // ============================
    // 4) 色の合成
    // ============================
    float3 col = baseCol;

    // 少し黄色味の月
    float3 moonColor = float3(0.98f, 0.95f, 0.88f);
    float3 haloColor = float3(0.25f, 0.30f, 0.45f);

    col += moonColor * moonMask * 1.2f; // 明るさ調整
    col += haloColor * haloMask * 0.7f;
    col += horizonGlow;

    return float4(saturate(col), 1.0f);
}
