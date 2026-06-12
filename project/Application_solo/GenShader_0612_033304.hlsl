// 時間やカメラ情報を取得するための構造体 (register b2)
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

// 入力構造体として VertexShaderOutput を定義し、使用してください。
struct VertexShaderOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

// ShaderToy風エフェクトの注意事項に準拠するヘルパー関数
// GLSLの mod と同等の挙動をする関数 (今回は未使用だが、互換性のため残す)
float2 mod_glsl(float2 x, float2 y) {
    return x - y * floor(x / y);
}

// 安全なハッシュ関数
float hash12_safe(float2 p) {
    float3 p3  = frac(float3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.x + p3.y) * p3.z);
}

// メイン関数（エントリーポイント）
float4 main(VertexShaderOutput input) : SV_TARGET0 {
    // 画面中央を原点とするUV座標に変換 [-1, 1]
    float2 uv = input.texcoord * 2.0 - 1.0;
    
    // アスペクト比は考慮せず、正方形の空間として扱う（エンジン仕様に解像度情報がないため）
    // float aspectRatio = 1920.0 / 1080.0; // 例: ユーザーがこの値を直接シェーダーに渡すか、固定値
    // uv.x *= aspectRatio; // 必要に応じてコメント解除

    float time = gPerFrame.time;

    float dist = length(uv); // 中心からの距離
    float angle = atan2(uv.y, uv.x); // 中心からの角度

    // 波紋のパラメータ (ランダムシード 1781202784.3687189_7014 からインスピレーション)
    float baseFrequency = 12.0; // 基本の波紋の粗さ
    float waveSpeed = 1.0;      // 波紋が広がる速さ
    float decayStartDist = 0.2; // 中央から減衰が始まる距離
    float decayRate = 5.0;      // 減衰の速さ
    float distortionStrength = 0.04; // UV歪みの最大強度
    float noiseScale = 30.0;    // 波紋に乗せるノイズのスケール

    // 波紋の高さマップを生成
    // 複数のサイン波を重ねて複雑な波紋を作ることで、有機的な揺らぎを表現
    float waveHeight = 0.0;

    // 第一の波紋レイヤー：基本的な広がりと減衰
    float wave1 = sin(dist * baseFrequency - time * waveSpeed);
    // 第二の波紋レイヤー：より細かいディテールと速度差
    float wave2 = sin(dist * (baseFrequency * 1.5) - time * (waveSpeed * 1.2) + angle * 0.5);
    // 第三の波紋レイヤー：微細な擾乱と角度による変化
    float wave3 = sin(dist * (baseFrequency * 2.5) - time * (waveSpeed * 0.8) - angle * 0.3);

    // 波紋の組み合わせ
    waveHeight = wave1 * 0.6 + wave2 * 0.3 + wave3 * 0.1;
    
    // 距離に応じた減衰
    float decayFactor = saturate(1.0 - pow(max(0.0, dist - decayStartDist) * decayRate, 1.5));
    waveHeight *= decayFactor;

    // 時間によって変化するノイズを波紋に加える
    float2 noiseCoord = uv * noiseScale + time * 0.1;
    float noise = hash12_safe(noiseCoord) * 2.0 - 1.0; // [-1, 1] の範囲
    waveHeight += noise * 0.1 * decayFactor; // ノイズも距離と共に減衰させる

    // 波紋の強度を正規化し、滑らかにする
    waveHeight = (waveHeight * 0.5 + 0.5); // [0, 1] に正規化
    waveHeight = pow(waveHeight, 1.5); // よりシャープなピークと滑らかな谷

    // 波紋の高さを使ってUVを歪ませる
    float2 distortedUV = input.texcoord;
    float2 waveNormal = float2(
        ddx(waveHeight), // 波紋のX方向の勾配
        ddy(waveHeight)  // 波紋のY方向の勾配
    );
    // 波紋の法線に基づいた歪み
    distortedUV += waveNormal * distortionStrength * decayFactor;

    // 色収差のような効果を狙って、RGBチャンネルを個別にオフセット
    // 波紋の勾配が急なほど、色ずれが大きくなるように見せる
    float2 offsetR = distortedUV + waveNormal * distortionStrength * 0.03 * decayFactor;
    float2 offsetG = distortedUV; // 緑チャンネルは中心に
    float2 offsetB = distortedUV - waveNormal * distortionStrength * 0.03 * decayFactor;

    // 背景テクスチャをサンプリング
    float4 colR = gTexture.Sample(gSampler, offsetR);
    float4 colG = gTexture.Sample(gSampler, offsetG);
    float4 colB = gTexture.Sample(gSampler, offsetB);

    // 最終的な色を合成（色収差効果）
    float4 finalColor = float4(colR.r, colG.g, colB.b, 1.0); // アルファは不透明として扱う

    // 波紋の高さに応じて色を調整
    // ピーク部分を明るく、谷部分を暗くする
    float brightness = lerp(0.6, 1.4, waveHeight); // 波紋の高さで明るさを調整
    finalColor.rgb *= brightness;

    // 水面の色調を加える (波紋が強いほど水色に)
    float3 waterTint = float3(0.05, 0.15, 0.35); // 深い青緑色
    finalColor.rgb = lerp(finalColor.rgb, waterTint, waveHeight * 0.4);

    // ハイライト効果 (波紋の勾配が急な部分に光沢を加える)
    float normalStrength = length(waveNormal);
    float highlight = pow(saturate(normalStrength * 5.0 - 0.5), 4.0) * 0.8; // ピークを強調し、よりシャープに
    finalColor.rgb += highlight * float3(0.9, 0.95, 1.0); // 白っぽいハイライト

    // Premultiplied Alpha
    // 発光エフェクトの場合、RGBに発光色を乗算し、アルファは適切に設定する。
    // 今回は水面エフェクトであり、背景テクスチャを歪ませて色調を調整しているため、
    // アルファは基本的に1.0 (不透明) として扱い、RGBに最終的な色を乗算する。
    finalColor.a = 1.0; 

    return float4(finalColor.rgb * finalColor.a, finalColor.a);
}