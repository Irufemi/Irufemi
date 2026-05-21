#include "Object3d.hlsli"
#include "Noise.hlsli"

/**
 * @file ExplosionFlame.PS.hlsl
 * @brief シリンダー表面を這う爆炎・マグマ状のエフェクトを行うピクセルシェーダー
 */

// 特殊パラメータ (register b6 / RootSlot::Special)
struct ExplosionParams {
    float32_t4 edgeColor;
    float32_t4 midColor;
    float32_t4 coreColor;
    float32_t speed;
    float32_t intensity;
    float32_t noiseScale;
    float32_t erosion;
    float32_t pad[48];
};
ConstantBuffer<ExplosionParams> gExplosion : register(b6);

#include "PerFrame.hlsli"
ConstantBuffer<PerFrameData> gPerFrame : register(b2);

struct PixelShaderOutput {
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    
    float2 uv = input.texcoord;
    float time = gPerFrame.time * gExplosion.speed;
    
    // ワールド座標ベースでノイズをサンプリング
    // Y座標からtimeを引くことで、見た目上は炎が下から上へ昇る
    float3 pSurf = input.worldPosition * 0.5;
    pSurf.y -= time * 2.0;
    
    // フラクタルブラウン運動(fBm)による炎のうねり生成
    // 横方向（X/Z）への大きなスクロールを無くし、純粋に「下から上」へ昇りつつ
    // 少しだけ揺らぐ（boiling）ように調整
    float noiseVal = fBm(pSurf * gExplosion.noiseScale + float3(0.0, time * 0.1, 0.0));
    
    // 下部（根元）ほど燃え盛り、上に行くほど消えやすくするマスクを法線(Y軸)を使って作成
    // 今回の爆風は寝かせた円柱なので、normal.y が -1(下向き)～1(上向き) になる。
    float heightGradient = saturate(input.normal.y * 0.5 + 0.5); // 0.0(根元) ～ 1.0(上部)

    // 根元は浸食(erosion)を弱くし、常に地表から炎が湧き出ているようにする
    // ただし完全にゼロにするとプラスチックの管のように不自然になるため、根元でも少しは削れるように(0.2)する
    float localErosion = gExplosion.erosion * lerp(0.2, 1.0, heightGradient);
    
    // 浸食によるアルファマスクと形状の削れ
    float alphaMask = smoothstep(localErosion, localErosion + 0.3, noiseVal);
    
    // さらに根元部分は不透明度を底上げするが、ベタ塗りにならないよう程々に(0.5倍)調整
    alphaMask = max(alphaMask, (1.0 - heightGradient) * (1.0 - gExplosion.erosion) * 0.5);

    // 円柱の両端（爆風の先端）に向かってフェードアウトさせる
    float edgeFade = sin(input.texcoord.y * 3.14159265);
    alphaMask *= edgeFade;
    
    // ノイズの強さと高さに応じて色をブレンド（外側→中間→芯）
    // 根元ほどコア色(芯)になりやすくする
    float coreBlend = smoothstep(0.6, 1.0, noiseVal) + (1.0 - heightGradient) * 0.5;
    coreBlend = saturate(coreBlend);

    float3 color = lerp(gExplosion.edgeColor.rgb, gExplosion.midColor.rgb, smoothstep(0.2, 0.5, noiseVal));
    color = lerp(color, gExplosion.coreColor.rgb, coreBlend);
    
    float finalAlpha = alphaMask * gExplosion.edgeColor.a;
    
    if (finalAlpha <= 0.01) { discard; }
    
    output.color = float32_t4(color * gExplosion.intensity, finalAlpha);
    
    return output;
}
