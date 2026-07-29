#include "Fullscreen.hlsli"
#include "Bindless.hlsli"
#include "PostProcessBindlessParams.hlsli"

SamplerState gSampler : register(s0);

struct DissolveParams {
    float32_t4 edgeColor;
    float32_t4 backgroundColor; // 追加：C++側との位置合わせのため
    float32_t threshold;
    float32_t edgeRange;
    int32_t noiseType;
};

ConstantBuffer<DissolveParams> gParams : register(b0);



PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;

    // マスク（ノイズ）をサンプリング
    float32_t mask = gExtraTexture.Sample(gSampler, input.texcoord).r;

    // 閾値以下ならピクセルを棄却
    if (mask <= gParams.threshold) {
        discard;
    }

    // メインテクスチャをサンプリング
    output.color = gTexture.Sample(gSampler, input.texcoord);

    // エッジ部分のハイライト
    // threshold ~ threshold + edgeRange の範囲を 1.0 ~ 0.0 に変換
    float32_t edge = 1.0f - smoothstep(gParams.threshold, gParams.threshold + gParams.edgeRange, mask);
    
    // エッジっぽいほど指定した色を加算（発光感）
    output.color.rgb += edge * gParams.edgeColor.rgb;

    return output;
}
