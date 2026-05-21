/**
 * @file BombCore.PS.hlsl
 * @brief 敵の十字爆撃用の弾コアシェーダ（不安定なマグマコア＋十字亀裂）
 */

#include "Object3d.hlsli"
#include "Lighting.hlsli"
#include "PerFrame.hlsli"

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<LightCommonData> gLightCommon : register(b1);
ConstantBuffer<PerFrameData> gPerFrame : register(b2);

struct BombCoreParams {
    float4 edgeColor;      // フチ（外側）の色
    float4 coreColor;      // 中心（内側）の色
    float4 crackColor;     // 亀裂から漏れ出る光の色
    float noiseScale;      // ノイズのスケール
    float distortion;      // 亀裂の歪み具合
    float pulseSpeed;      // 明滅の速度
    float intensity;       // 全体の発光強度
    float padding;         // 16バイトアライメント用
};
ConstantBuffer<BombCoreParams> gBombCore : register(b6);

#include "Noise.hlsli"

struct PixelShaderOutput {
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) 
{
    PixelShaderOutput output;
    
    float time = gPerFrame.time;
    float pulse = (sin(time * gBombCore.pulseSpeed) + 1.0f) * 0.5f; // 0.0 ~ 1.0

    // 球体の法線を3D座標として利用（ローカル座標系と同等）
    float3 p = normalize(input.normal);
    
    // ノイズによる歪みの計算
    // 時間経過でウネウネと動かす
    float n = noise(p * gBombCore.noiseScale + time * 2.0f);
    float3 distortedP = p + (n - 0.5f) * gBombCore.distortion;
    distortedP = normalize(distortedP);

    // ==========================================
    // 1. ベースのマグマコア（フレネル反射）
    // ==========================================
    // 視線ベクトルと法線の内積（フチほど0に近づく）
    float3 viewDir = normalize(gPerFrame.cameraWorldPosition - input.worldPosition);
    float rim = 1.0f - saturate(dot(viewDir, input.normal));
    
    // ベースカラーは中心(core)からフチ(edge)へグラデーション
    float3 baseColor = lerp(gBombCore.coreColor.rgb, gBombCore.edgeColor.rgb, pow(rim, 2.0f));

    // ==========================================
    // 2. 十字の亀裂 (Cross Crack)
    // ==========================================
    // X, Y, Z軸に沿った亀裂を作成
    // 絶対値が0に近いほど軸に近い（=亀裂の中心）
    float crackThickness = 0.08f + 0.05f * pulse; // パルスで亀裂が開閉する
    
    float crackX = 1.0f - smoothstep(0.0f, crackThickness, abs(distortedP.x));
    float crackY = 1.0f - smoothstep(0.0f, crackThickness, abs(distortedP.y));
    float crackZ = 1.0f - smoothstep(0.0f, crackThickness, abs(distortedP.z));
    
    // 最も強い亀裂を採用
    float crackIntensity = max(max(crackX, crackY), crackZ);
    
    // ==========================================
    // 3. ブレンドと最終出力
    // ==========================================
    // 亀裂部分は強く発光する
    float3 finalColor = baseColor * (0.2f + 0.8f * n); // マグマのムラ
    finalColor += gBombCore.crackColor.rgb * crackIntensity * (1.0f + pulse * 2.0f);
    
    finalColor *= gBombCore.intensity;

    output.color = float4(saturate(finalColor), gMaterial.color.a);
    return output;
}
