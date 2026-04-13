#include "Object3d.hlsli"
#include "LightningUtility.hlsli"

/**
 * @file LightningCrawl.PS.hlsl
 * @brief モデル表面を電撃が這う表現を行うピクセルシェーダー
 */

// 特殊パラメータ (register b6 / RootSlot::Special)
struct LightningParams {
    float32_t4 color;           //!< 電撃の色 (HDR対応)
    float32_t speed;            //!< アニメーション速度
    float32_t intensity;        //!< 輝きの強さ
    float32_t noiseScale;       //!< ノイズの密度
    float32_t noiseThreshold;    //!< 雷のしきい値
};
ConstantBuffer<LightningParams> gLightning : register(b6);

// カメラ情報 (register b2 / RootSlot::Camera)
struct Camera {
    float32_t4x4 view;
    float32_t4x4 projection;
    float32_t3 worldPosition;
    float32_t time;
    float32_t deltaTime;
};
ConstantBuffer<Camera> gCamera : register(b2);

struct PixelShaderOutput {
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;

    // UVをベースに、時間でアニメーションさせる
    float2 uv = input.texcoord;
    float32_t time = gCamera.time * gLightning.speed;

    // 電撃のパターンを fBm ノイズで生成
    // 単一のノイズを加工して、より太く、視認性の高い稲妻にします
    float32_t n1 = fBm(uv * gLightning.noiseScale + float32_t2(time * 0.5, time * 0.2));
    
    // 稲妻の形状（しきい値周辺を盛り上げる）
    // abs(n1 - threshold) が 0 に近いほど 1.0 に近づく
    float32_t bolt = 1.0 - saturate(abs(n1 - gLightning.noiseThreshold) / 0.1);
    
    // さらにノイズを重ねてキラキラさせる
    float32_t sparkle = fBm(uv * gLightning.noiseScale * 8.0 - float32_t2(time * 2.0, time));
    bolt *= (0.7 + 0.3 * sparkle);

    // 芯の部分だけ色を乗せる
    float32_t4 color = gLightning.color * gLightning.intensity;
    float32_t alpha = bolt * gLightning.color.a;
    
    // 完全に透明でない限り discard しない（デバッグのため）
    if (alpha <= 0.001) { discard; }

    output.color = float32_t4(color.rgb * bolt, alpha);

    // 加算合成を想定しているため、背景との馴染ませ方は PSO に任せる
    return output;
}
