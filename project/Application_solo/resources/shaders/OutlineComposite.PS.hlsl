#include "PerFrame.hlsli"

Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);

ConstantBuffer<PerFrameData> gPerFrame : register(b2);

struct PixelInput {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 main(PixelInput input) : SV_TARGET {
    float2 offset = 1.0f / gPerFrame.resolution;
    
    // 画面端のピクセルでは強制的にエッジを無視する（画面ふちに線が出るのを防ぐ）
    if (input.uv.x <= offset.x || input.uv.x >= 1.0f - offset.x ||
        input.uv.y <= offset.y || input.uv.y >= 1.0f - offset.y) {
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    // 単純な十字サンプリングによる膨張（ブラー・エッジ検出）
    float center = tex.Sample(smp, input.uv).r;
    float up     = tex.Sample(smp, input.uv + float2(0, -offset.y)).r;
    float down   = tex.Sample(smp, input.uv + float2(0,  offset.y)).r;
    float left   = tex.Sample(smp, input.uv + float2(-offset.x, 0)).r;
    float right  = tex.Sample(smp, input.uv + float2( offset.x, 0)).r;
    
    // エッジを検出 (周囲が白で中心が黒、またはその逆)
    float edge = abs((up + down + left + right) - center * 4.0f);
    
    // エッジの強さに応じてオレンジ色 (1.0, 0.5, 0.0) を出力する（背景は edge=0 で透明になる）
    return float4(1.0f * edge, 0.5f * edge, 0.0f, edge);
}
