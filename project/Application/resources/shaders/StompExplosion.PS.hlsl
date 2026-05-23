#include "Object3d.hlsli"
#include "Noise.hlsli"
#include "PerFrame.hlsli"

/**
 * @file StompExplosion.PS.hlsl
 * @brief ストンプ攻撃の大噴火用シェーダー (Raymarching Volumetric)
 *        中身の詰まった立体的な爆発を描画します。
 */

struct ExplosionParams {
    float32_t4 edgeColor;
    float32_t4 midColor;
    float32_t4 coreColor;
    float32_t speed;
    float32_t intensity;
    float32_t noiseScale;
    float32_t erosion;
    float3 sphereCenter;
    float sphereRadius;
    float pad[44];
};
ConstantBuffer<ExplosionParams> gExplosion : register(b6);
ConstantBuffer<PerFrameData> gPerFrame : register(b2);

struct PixelShaderOutput {
    float32_t4 color : SV_TARGET0;
};

// 球とレイの交差判定
bool intersectSphere(float3 ro, float3 rd, float3 center, float radius, out float t0, out float t1) {
    t0 = 0.0;
    t1 = 0.0;
    float3 oc = ro - center;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - radius * radius;
    float h = b * b - c;
    if (h < 0.0) return false; // 交差しない
    h = sqrt(h);
    t0 = -b - h; // 入口
    t1 = -b + h; // 出口
    return true;
}

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;

    float3 ro = gPerFrame.cameraWorldPosition;
    float3 rd = normalize(input.worldPosition - ro);

    float t0, t1;
    if (!intersectSphere(ro, rd, gExplosion.sphereCenter, gExplosion.sphereRadius, t0, t1)) {
        discard;
    }

    // カメラが球体の内部にある場合は、現在位置(t=0)からマーチングを開始する
    t0 = max(t0, 0.0);

    // レイが球体内を進む距離
    float tMax = t1 - t0;
    if (tMax <= 0.0) discard;

    // --- レイマーチング設定 ---
    const int STEPS = 32; // サンプリング回数 (RTX 3060想定なら32〜64で余裕)
    float stepSize = tMax / float(STEPS);
    float time = gPerFrame.time * gExplosion.speed;
    float scale = 0.05 * gExplosion.noiseScale;

    float4 accumColor = float4(0, 0, 0, 0);
    float tCurrent = t0;

    for (int i = 0; i < STEPS; i++) {
        float3 pos = ro + rd * tCurrent;

        // 中心からの距離による正規化 (0.0 = 中心, 1.0 = 球面)
        float distToCenter = distance(pos, gExplosion.sphereCenter);
        float distNorm = saturate(distToCenter / gExplosion.sphereRadius);

        // --- UV / サンプリング座標の計算 ---
        // 放射状に広がる動き
        float3 dirFromCenter = normalize(pos - gExplosion.sphereCenter);
        float3 pNoise = pos * scale - dirFromCenter * time * 2.0;
        
        // 熱対流（上方向への揺らぎ）
        pNoise += float3(0.0, time * 0.5, 0.0);

        // ノイズサンプリング
        float noiseVal = fBm(pNoise);

        // --- 密度の計算 ---
        // 外側(表面)に近づくほど、または時間(erosion)が経つほど侵食される
        float localErosion = gExplosion.erosion + (distNorm * 0.6);
        float density = smoothstep(localErosion, localErosion + 0.3, noiseVal);

        // 上部ほど消えやすくする（元のシェーダーの特性を継承）
        float yGradient = saturate((pos.y - gExplosion.sphereCenter.y) / gExplosion.sphereRadius);
        density *= smoothstep(0.0, 0.5, 1.0 - (yGradient * gExplosion.erosion));

        // 完全な球体の境界がパキッと見えないように、外縁部をフェードアウト
        density *= smoothstep(1.0, 0.8, distNorm);

        if (density > 0.01) {
            // --- 色の計算 ---
            float coreBlend = smoothstep(0.4, 0.8, noiseVal);
            float3 color = lerp(gExplosion.edgeColor.rgb, gExplosion.midColor.rgb, smoothstep(0.1, 0.5, noiseVal));
            color = lerp(color, gExplosion.coreColor.rgb, coreBlend);

            // このステップでのアルファ（不透明度）
            float alpha = density * 0.25; // ここで全体の煙の「濃さ」を調整

            // Pre-multiplied alpha合成
            color *= alpha;
            accumColor.rgb += color * (1.0 - accumColor.a);
            accumColor.a += alpha * (1.0 - accumColor.a);

            // 完全に不透明になったらそれ以上奥は計算しなくてよい（早期リターン）
            if (accumColor.a > 0.99) break;
        }

        tCurrent += stepSize;
    }

    // --- 最終合成 ---
    if (accumColor.a <= 0.01) discard;

    // 近接フェード (カメラが近すぎるときのクリッピングを防ぎつつ柔らかくする)
    float distToCamera = distance(gPerFrame.cameraWorldPosition, input.worldPosition);
    float distanceFade = smoothstep(10.0, 30.0, distToCamera);

    // 強度(intensity)をかけて出力
    output.color = float32_t4(accumColor.rgb * gExplosion.intensity, accumColor.a * distanceFade);

    return output;
}
