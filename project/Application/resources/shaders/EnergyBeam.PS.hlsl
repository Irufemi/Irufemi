#include "Object3d.hlsli"
#include "Noise.hlsli"
#include "PerFrame.hlsli"

/**
 * @file EnergyBeam.PS.hlsl
 * @brief 高エネルギーレーザービーム用シェーダー
 *        中心の強力なコアと、周囲に猛スピードで流れるエネルギーの奔流を表現します。
 */

// LightningCrawlと互換性を持たせるため、同じパラメータ構造体を利用
struct BeamParams {
    float32_t4 color;           //!< 外周オーラの色
    float32_t4 coreColor;       //!< コア（中心）の色
    float32_t speed;            //!< 進行速度
    float32_t intensity;        //!< オーラの輝度
    float32_t noiseScale;       //!< ノイズの細かさ
    float32_t noiseThreshold;   //!< ノイズのしきい値（未使用だが互換用）
    float32_t coreIntensity;    //!< コアの輝度
    float32_t coreThreshold;    //!< コアの太さ
    float32_t coreScale;        //!< 未使用
    float32_t spinSpeed;        //!< 横回転(螺旋)の速度
    float32_t twistScale;       //!< 螺旋のねじれの強さ
    float32_t pad[3];
};
ConstantBuffer<BeamParams> gBeam : register(b6);
ConstantBuffer<PerFrameData> gPerFrame : register(b2);

struct PixelShaderOutput {
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    float2 uv = input.texcoord;
    float time = gPerFrame.time * gBeam.speed * 2.0; // より高速に

    // 視線と法線の内積（フレネル）を使用して、円柱のフチ（輪郭）と中心（コア）を分ける
    float3 V = normalize(gPerFrame.cameraWorldPosition - input.worldPosition);
    float3 N = normalize(input.normal);
    float fresnel = saturate(dot(N, V)); // 真正面が1.0、フチが0.0

    // --- 1. 根本と先端のフェード (Cylinderのフタを隠す) ---
    // uv.y は根元から先端へ 0.0 ~ 1.0 になる。両端をフェードアウトさせる
    float verticalFade = smoothstep(0.0, 0.05, uv.y) * (1.0 - smoothstep(0.95, 1.0, uv.y));
    
    // --- 2. ノイズスクロール (奔流の表現) ---
    // 円筒の周囲をシームレスにするため角度計算（スピンとねじれを加算）
    float angle = uv.x * TAU + (gPerFrame.time * gBeam.spinSpeed) + (uv.y * gBeam.twistScale);
    
    // uv.y 方向に猛スピードで引くことで、前方にエネルギーが流れる
    float3 p1 = float3(cos(angle), sin(angle), uv.y * 3.0 - time * 3.0);
    float3 p2 = float3(cos(angle)*1.5, sin(angle)*1.5, uv.y * 5.0 - time * 5.0);
    
    float noise1 = fBm(p1 * gBeam.noiseScale);
    float noise2 = fBm(p2 * gBeam.noiseScale * 1.5);
    float combinedNoise = (noise1 * 0.6 + noise2 * 0.4);

    // --- 3. オーラとコアの合成 ---
    // コアは中心（フレネルが強い部分）ほど白く輝く。
    // coreThreshold で太さを調整。
    float coreFactor = pow(fresnel, max(0.1, 10.0 - gBeam.coreThreshold * 5.0));
    
    // 激しいノイズを纏ったオーラ
    // フチにいくほどフレネルは小さくなるが、ノイズによって乱れた炎のように見える
    float auraFactor = combinedNoise * (1.0 - pow(fresnel, 3.0)); 

    float3 finalColor = 0;
    
    // オーラレイヤー
    finalColor += gBeam.color.rgb * gBeam.intensity * auraFactor;
    
    // コアレイヤー
    // コアにも少しノイズを混ぜて、完全に単調にならないようにする
    float corePulse = 0.8 + 0.2 * noise1;
    finalColor += gBeam.coreColor.rgb * gBeam.coreIntensity * coreFactor * corePulse;

    // --- 4. 最終アルファの計算 ---
    // コアとオーラを合わせた不透明度
    float alpha = saturate((auraFactor + coreFactor) * verticalFade);
    
    if (alpha <= 0.01) {
        discard;
    }

    // BlendAdd で描画されることを前提とするため、color のアルファ乗算は描画パス（PSO）に依存。
    // ここでは単純に輝度を出力
    output.color = float32_t4(finalColor, alpha * gBeam.color.a);

    return output;
}
