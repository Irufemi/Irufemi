#include "Fullscreen.hlsli"

struct NoiseParams {
    float intensity;
    float time;
};

ConstantBuffer<NoiseParams> gParams : register(b0);
Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

float rand2dTo1d(float2 value) {
    float2 dot_res = dot(value, float2(12.9898, 78.233));
    return frac(sin(dot_res.x) * 43758.5453);
}

struct PixelShaderOutput {
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    
    // 元の色を取得
    float32_t4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    
    // 乱数生成（座標と時間を使用）
    // 資料では Seed 値のみで決まるとされているので、時間に一定の係数をかけて変化を分かりやすくする
    float32_t random = rand2dTo1d(input.texcoord * (gParams.time + 1.0f));
    
    // ノイズを適用（資料の Congratulations!! 画面では乗算しているように見える）
    // random は 0~1 なので、強度（Intensity）で影響度を調整できるようにする
    float32_t noise = lerp(1.0f, random, gParams.intensity);
    
    output.color.rgb = textureColor.rgb * noise;
    output.color.a = textureColor.a;
    
    return output;
}
