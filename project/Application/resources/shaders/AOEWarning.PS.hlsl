#include "./Object3d.hlsli"
#include "./Material.hlsli"
#include "PerFrame.hlsli"

/**
 * @file AOEWarning.PS.hlsl
 * @brief AOE（Area of Effect）予兆用のシェーダー
 *        円形（波紋）や直線状の流れなどを表現し、危険範囲をプレイヤーに通知します。
 */

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<PerFrameData> gPerFrame : register(b2);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    // UV座標 (0.0 ~ 1.0)
    float2 uv = input.texcoord;
    
    // MaterialのuvTransformにパラメータを仕込んでいる想定
    // _11 = 形状タイプ (0: 円形/Radial, 1: 直線/Linear)
    // _12 = 警告の進行度合 (0.0 ~ 1.0)
    int shapeType = (int)gMaterial.uvTransform[0][0];
    float warningRatio = gMaterial.uvTransform[0][1];

    float alphaMult = 1.0;
    float time = gPerFrame.time;

    if (shapeType == 0) {
        // --- 円形 (Radial) ---
        float dist = length(uv - 0.5) * 2.0;
        
        if (dist > 1.0) {
            discard;
        }
        
        // 内側から外側へ広がる波
        // dist は 0.0(中心) 〜 1.0(外側)
        // マイナス時間で引くことで、中心から外側へ波が向かう
        float wave = frac(dist * 2.0 - time * 1.5);
        
        // 警告の終盤ほど赤く、全体が明るくなるように
        float baseAlpha = lerp(0.3, 0.8, warningRatio);
        
        // 波の強さ
        alphaMult = wave * baseAlpha;
        
        // 円の境界線をはっきりさせる
        if (dist > 0.95) {
            alphaMult = 1.0;
        }

    } else { // shapeType == 1 (Linear)
        // --- 直線 (Linear) ---
        // V軸(y方向)に向かって流れるラインを表現
        // time をプラスすることで、uv.y が小さい方（+Z/前方）へ波が向かう
        float wave = frac(uv.y * 2.5 + time * 1.5);
        
        // エッジフェード (smoothstepのmin < maxを守る)
        float edgeU = smoothstep(0.0, 0.1, uv.x) * (1.0 - smoothstep(0.9, 1.0, uv.x));
        float edgeV = smoothstep(0.0, 0.1, uv.y) * (1.0 - smoothstep(0.9, 1.0, uv.y));
        
        float baseAlpha = lerp(0.3, 1.0, warningRatio);
        alphaMult = wave * edgeU * edgeV * baseAlpha;
    }

    // 最終カラー計算
    output.color = gMaterial.color;
    output.color.a *= alphaMult;
    
    // 完全透明なら棄却
    if (output.color.a <= 0.01) {
        discard;
    }

    return output;
}
