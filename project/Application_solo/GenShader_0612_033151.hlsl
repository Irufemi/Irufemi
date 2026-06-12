// 厳格なエンジン仕様に準拠する入力・出力構造体
struct VertexShaderOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

// 厳格なエンジン仕様に準拠する定数バッファとリソース
struct PerFrameData {
    float4x4 view;
    float4x4 projection;
    float3 cameraWorldPosition;
    float time;        // アニメーション用の時間
    float deltaTime;
    float2 padding;
};
ConstantBuffer<PerFrameData> gPerFrame : register(b2);

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

// 安全なmod関数 (GLSL風) - float版を追加
float mod_glsl(float x, float y) {
    return x - y * floor(x / y);
}

float2 mod_glsl(float2 x, float2 y) {
    return x - y * floor(x / y);
}

// 安全なハッシュ関数
float hash12_safe(float2 p) {
    float3 p3  = frac(float3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.x + p3.y) * p3.z);
}

float4 main(VertexShaderOutput input) : SV_TARGET0 {
    float2 uv = input.texcoord;
    float time = gPerFrame.time;

    // 基本となる背景色（元テクスチャを少し暗く歪ませて使用）
    float2 distorted_uv = uv;
    distorted_uv += sin(uv.yx * 15.0 + time * 2.0) * 0.015 * hash12_safe(uv * 7.7 + time * 0.5);
    float3 base_color = gTexture.Sample(gSampler, distorted_uv).rgb * 0.4;

    // ----------------------------------------------------
    // 1. 細かい高速走査線エフェクト
    // ----------------------------------------------------
    float scanline_y1 = frac(uv.y * 70.0 + time * 5.0);
    float scanline_intensity1 = smoothstep(0.0, 0.05, scanline_y1) * smoothstep(1.0, 0.95, scanline_y1);
    float3 scanline_color1 = float3(0.1, 0.4, 0.5) * scanline_intensity1 * 1.5; // シアン系

    // ----------------------------------------------------
    // 2. 太くゆっくり動くブロック状走査線とノイズ
    // ----------------------------------------------------
    // float2を返すmod_glslをfloatに代入していたため、float版のmod_glslを定義し、そちらが使用されるように修正
    float block_scan_y = mod_glsl(uv.y * 10.0 - time * 0.8, 1.0);
    float block_intensity = step(0.0, block_scan_y) * step(block_scan_y, 0.2); // 太いブロックを生成

    // ブロック内部にランダムなノイズ
    float2 block_id = floor(uv * float2(20.0, 10.0) + time * 0.5);
    float block_noise = hash12_safe(block_id) * 0.5 + 0.5; // 0.5-1.0の範囲
    float3 block_color = float3(0.5, 0.1, 0.4) * block_noise; // マゼンタ系

    // ノイズとブロック走査線を合成
    float3 glitch_block = block_color * block_intensity * (sin(time * 10.0 + uv.x * 20.0) * 0.5 + 0.5);

    // ----------------------------------------------------
    // 3. ピクセル化されたグリッチノイズとRGBシフト
    // ----------------------------------------------------
    float2 pixel_uv = floor(uv * float2(80.0, 60.0)) / float2(80.0, 60.0);
    float pixel_noise_val = hash12_safe(pixel_uv * 13.0 + time * 7.0);

    float3 pixel_noise_color = float3(0, 0, 0);
    // ランダムな位置に短い輝点ノイズ
    if (pixel_noise_val > 0.95) {
        pixel_noise_color = float3(0.8, 0.7, 0.2) * (pixel_noise_val - 0.95) * 20.0; // 黄色-オレンジ系
    }

    // RGBチャンネルのランダムなオフセット
    float2 shift_offset = float2(hash12_safe(uv * 99.0 + time * 12.0) - 0.5, hash12_safe(uv * 77.0 + time * 13.0) - 0.5) * 0.005;
    float r = gTexture.Sample(gSampler, uv + shift_offset * float2(1.0, 0.0)).r;
    float g = gTexture.Sample(gSampler, uv + shift_offset * float2(0.0, 1.0)).g;
    float b = gTexture.Sample(gSampler, uv - shift_offset * float2(1.0, 1.0)).b;
    float3 rgb_shifted_color = float3(r, g, b) * 0.3; // シフトしたテクスチャは控えめに

    // 特定のタイミングで強いグリッチ歪み
    float glitch_factor = sin(time * 3.0) * 0.5 + 0.5; // 0.0から1.0
    if (glitch_factor > 0.8) {
        float2 glitch_uv_offset = float2(hash12_safe(uv * 10.0 + time * 20.0) - 0.5, hash12_safe(uv * 20.0 + time * 25.0) - 0.5) * 0.1 * (glitch_factor - 0.8) * 5.0;
        rgb_shifted_color = gTexture.Sample(gSampler, uv + glitch_uv_offset).rgb;
        rgb_shifted_color *= float3(1.0, 0.5, 0.8); // 歪んだ部分を赤紫に強調
    }


    // ----------------------------------------------------
    // 最終的な合成
    // ----------------------------------------------------
    float3 final_color = base_color;
    final_color += scanline_color1;
    final_color += glitch_block;
    final_color += pixel_noise_color;
    final_color += rgb_shifted_color * 0.5; // RGBシフトした色を少し加える

    // 全体のノイズを追加
    float overall_noise = hash12_safe(uv * 50.0 + time * 15.0) * 0.1;
    final_color += overall_noise * float3(0.1, 0.1, 0.1);

    // グリッチの強さを時間で調整
    float effect_strength = 1.0;
    // float effect_pulsate = sin(time * 0.5) * 0.5 + 0.5; // 0.0 to 1.0
    // final_color = lerp(gTexture.Sample(gSampler, uv).rgb * 0.7, final_color, effect_pulsate * 0.8 + 0.2);

    // 発光エフェクトのためにRGBに乗算済みアルファを適用
    float alpha = 1.0;
    return float4(final_color * alpha, alpha);
}