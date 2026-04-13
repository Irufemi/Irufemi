
/**
 * @file LightningUtility.hlsli
 * @brief 電撃表現用ノイズ・ユーティリティ関数
 */

// 擬似乱数 (2D -> 1D)
float rand(float2 n) { 
	return frac(sin(dot(n, float2(12.9898, 4.1414))) * 43758.5453);
}

// シンプルなノイズ (2D)
float noise(float2 p) {
	float2 ip = floor(p);
	float2 u = frac(p);
	u = u * u * (3.0 - 2.0 * u);
	
	float res = lerp(
		lerp(rand(ip), rand(ip + float2(1.0, 0.0)), u.x),
		lerp(rand(ip + float2(0.0, 1.0)), rand(ip + float2(1.0, 1.0)), u.x), u.y);
	return res * res;
}

// Fractal Brownian Motion (fBm)
float fBm(float2 p) {
	float f = 0.0;
	float a = 0.5;
	for (int i = 0; i < 4; i++) {
		f += a * noise(p);
		p *= 2.0;
		a *= 0.5;
	}
	return f;
}
