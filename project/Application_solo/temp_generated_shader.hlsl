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

struct VertexShaderOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

// GLSLライクなmod関数
float2 mod_glsl(float2 x, float2 y) {
    return x - y * floor(x / y);
}

// 安全なハッシュ関数
float hash12_safe(float2 p) {
    float3 p3  = frac(float3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.x + p3.y) * p3.z);
}

// 2D回転行列
float2x2 rotate2d(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return float2x2(c, -s, s, c);
}

float4 main(VertexShaderOutput input) : SV_TARGET0 {
    float2 uv = input.texcoord;

    // ランダムシードから派生した定数 (前回とは異なるアプローチのため)
    // 整数部と小数部を混ぜてユニークな値を生成
    float base_seed = 1781150192.9073164;
    float freq_multiplier = frac(base_seed * 0.123) * 3.0 + 5.0; // 周波数
    float speed_multiplier = frac(base_seed * 0.456) * 0.3 + 0.2; // 速度
    float distortion_strength_base = frac(base_seed * 0.789) * 0.03 + 0.02; // 歪みの基本強度
    float rotation_speed_base = frac(base_seed * 0.321) * 0.1 + 0.05; // 波の回転速度

    // UVを[-1, 1]の範囲に変換し、中心を原点に
    float2 centered_uv = uv * 2.0 - 1.0;

    // 時間経過で変化する歪みオフセットを計算
    float time = gPerFrame.time;

    // 複数の波を合成して複雑な歪みを生成
    float2 offset = 0;

    // 1. 螺旋状に広がる波
    float angle = atan2(centered_uv.y, centered_uv.x);
    float dist = length(centered_uv);
    float spiral_freq = freq_multiplier * 0.8;
    float spiral_speed = speed_multiplier * 1.5;
    float spiral_wave = sin(dist * spiral_freq + angle * 3.0 + time * spiral_speed);
    offset += float2(cos(angle + time * 0.3), sin(angle + time * 0.3)) * spiral_wave * 0.03;

    // 2. 放射状に広がる波
    float radial_freq = freq_multiplier * 1.2;
    float radial_speed = speed_multiplier * 1.0;
    float radial_wave = cos(dist * radial_freq - time * radial_speed);
    offset += normalize(centered_uv) * radial_wave * 0.02;

    // 3. 全体を横切る波
    float linear_freq_x = freq_multiplier * 0.5;
    float linear_freq_y = freq_multiplier * 0.7;
    float linear_speed = speed_multiplier * 0.8;
    float linear_wave_x = sin(centered_uv.x * linear_freq_x + time * linear_speed);
    float linear_wave_y = cos(centered_uv.y * linear_freq_y - time * linear_speed * 0.7);
    offset += float2(linear_wave_x, linear_wave_y) * 0.015;

    // 歪みの強さを時間で緩やかに脈動させる
    float distortion_pulse = (sin(time * 0.8) * 0.5 + 0.5);
    float total_distortion_strength = distortion_strength_base * (1.0 + distortion_pulse * 0.5);

    // 歪みの方向を時間で回転させる
    float rotation_angle = time * rotation_speed_base;
    offset = mul(rotate2d(rotation_angle), offset);

    // 歪みの中心からの距離に応じて強度を減衰させる (端で歪みが自然に消えるように)
    float falloff = 1.0 - pow(dist, 2.0); // distが0-sqrt(2)の範囲なので、中心が1、角が0になるように調整
    offset *= falloff * total_distortion_strength;

    // 歪んだUV座標を計算
    float2 distorted_uv = uv + offset;

    // 歪んだUVでテクスチャをサンプリング
    float4 texColor = gTexture.Sample(gSampler, distorted_uv);

    // 水中の色調補正と発光効果
    float3 finalColor = texColor.rgb;
    float finalAlpha = texColor.a;

    // 歪みの総量に基づいて色相をわずかに変化させる (青みや緑みを加える)
    float distortion_magnitude = length(offset);
    float hue_shift_factor = distortion_magnitude * 5.0; // 歪みが大きいほど変化
    
    // シンプルな色調変化 (青と緑のブレンド)
    float3 underwater_tint = lerp(float3(0.0, 0.1, 0.2), float3(0.0, 0.2, 0.1), (sin(time * 0.6) * 0.5 + 0.5));
    finalColor = lerp(finalColor, finalColor + underwater_tint, saturate(hue_shift_factor * 0.5));

    // 水面からの光の筋や泡のような発光効果
    float glow_factor_base = (sin(time * 2.0 + hash12_safe(uv) * 10.0) * 0.5 + 0.5) * 0.3; // 時間とUVに基づく脈動
    float glow_factor_distortion = distortion_magnitude * 10.0; // 歪み量が多いほど強く光る
    float total_glow_factor = saturate(glow_factor_base + glow_factor_distortion);

    // 発光色 (明るい青色)
    float3 glow_color = float3(0.6, 0.8, 1.0) * total_glow_factor;

    // Premultiplied Alphaの出力: RGBにアルファ値を乗算しない
    // 発光は元の色に加算
    finalColor += glow_color;
    finalAlpha = saturate(finalAlpha + total_glow_factor * 0.5); // アルファ値も発光に合わせて増加

    return float4(finalColor, finalAlpha);
}