// エンジン必須構造体
struct VertexShaderOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

// エンジン必須定数バッファ＆リソース
struct PerFrameData {
    float4x4 view;
    float4x4 projection;
    float3 cameraWorldPosition;
    float time;        // アニメーション用の時間
    float deltaTime;
    float2 padding;
};
ConstantBuffer<PerFrameData> gPerFrame : register(b2);

// メインテクスチャ (register t0) と サンプラー (register s0)
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

// ShaderToy風エフェクトを書く際の注意事項
// 1. mod関数の罠: GLSLの mod はマイナス値で挙動が反転しませんが、HLSLの fmod は反転します。
//    空間をリピートさせる場合は、必ず以下の安全な関数を定義して使ってください。
float2 mod_glsl(float2 x, float2 y) {
    return x - y * floor(x / y);
}

// 3. ノイズ（乱数）関数: sin と巨大な数値を掛ける古いGLSLハッシュ関数はGPU精度で真っ白になるバグを起こします。
//    以下の安全なハッシュ関数を使ってください。
float hash12_safe(float2 p) {
    float3 p3  = frac(float3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.x + p3.y) * p3.z);
}

// メインピクセルシェーダーのエントリーポイント
float4 main(VertexShaderOutput input) : SV_TARGET0 {
    float2 uv = input.texcoord;
    float2 center = float2(0.5, 0.5);
    float2 toCenter = uv - center;
    float dist = length(toCenter); // 中心からの距離
    float angle = atan2(toCenter.y, toCenter.x); // 中心からの角度

    float time = gPerFrame.time;

    // --- 波紋のレイヤー1: 大きくゆっくりとしたうねり ---
    // 距離、時間、角度に依存する複雑な波紋形状
    float freq1 = 20.0;
    float speed1 = 1.2;
    float wave1 = sin(dist * freq1 - time * speed1 + sin(angle * 3.0 + time * 0.7) * 0.3); 
    float decay1 = exp(-dist * 5.0); // 距離による減衰
    wave1 *= decay1;

    // --- 波紋のレイヤー2: 細かく速いさざ波 ---
    // 角度と時間でより複雑なオフセットを加え、有機的な動きに
    float freq2 = 55.0;
    float speed2 = 0.9;
    float offsetAngle = sin(angle * 7.0 + time * 0.4) * 0.2 + cos(dist * 10.0 + time * 0.6) * 0.1;
    float wave2 = sin(dist * freq2 + offsetAngle - time * speed2);
    float decay2 = exp(-dist * 7.0); // 距離による減衰を速く
    wave2 *= decay2;

    // --- 歪み計算 ---
    // 歪み方向をわずかに時間で揺らし、より流動的に
    float2 distortDir = normalize(toCenter + float2(sin(time * 0.3), cos(time * 0.4)) * 0.15);
    // 複数の波紋を合成して歪み量を決定
    float distortionAmount = (wave1 * 0.035 + wave2 * 0.012); 
    float2 distortedUV = uv + distortDir * distortionAmount;

    // テクスチャサンプリング
    float4 baseColor = gTexture.Sample(gSampler, distortedUV);

    // --- 色の調整と反射光の表現 ---
    float totalWave = (wave1 + wave2) * 0.5; // 波紋の総合的な強度

    // 波紋の谷とピークで輝度を調整
    float brightnessFactor = saturate(totalWave * 0.8 + 0.2); // 波紋が強いほど明るく
    baseColor.rgb *= brightnessFactor;

    // 反射光の色と強度
    float4 reflectionColor = float4(0.7, 0.9, 1.0, 1.0); // 青みがかったハイライト
    // 波紋のピークで反射を強く、急激に立ち上がるようにpowを使う
    float reflectionIntensity = pow(saturate(totalWave * 1.5 + 0.2), 6.0) * decay1 * 0.7; // totalWaveを強調してpow
    baseColor.rgb = lerp(baseColor.rgb, reflectionColor.rgb, reflectionIntensity);

    // --- 表面のきらめきノイズ ---
    // 時間と歪んだUVに基づいたノイズを生成
    float sparkleNoise = hash12_safe(distortedUV * 25.0 + time * 0.7);
    // ノイズを波紋のピーク付近で強調し、キラキラ感を出すために指数関数的に強くする
    sparkleNoise *= saturate(totalWave * 2.5 - 0.5); // totalWaveが0.5以上で出現
    sparkleNoise = pow(sparkleNoise, 12.0); // 非常に強く強調して点滅感を出す
    
    // 黄みがかったキラキラを加算
    baseColor.rgb += sparkleNoise * float3(1.0, 1.0, 0.8) * 0.6;

    // --- アルファ値の計算 ---
    // 中心で強く、外側へ向かうにつれて減衰。波紋の強度もアルファに影響
    float finalAlpha = saturate(1.0 - dist * 1.8 + totalWave * 0.4);
    // 時間経過で全体がゆっくり薄くなる効果（オプション、調整可能）
    finalAlpha *= saturate(1.0 - time * 0.05); 
    // 最低アルファを設定し、完全に消えないようにしつつ、波紋の強さで変化
    finalAlpha = lerp(0.1, 1.0, saturate(finalAlpha)); 
    
    // Premultiplied Alpha
    baseColor.rgb *= finalAlpha;
    baseColor.a = finalAlpha;

    return baseColor;
}