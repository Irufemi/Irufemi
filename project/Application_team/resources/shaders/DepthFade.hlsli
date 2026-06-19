#ifndef DEPTH_FADE_HLSLI
#define DEPTH_FADE_HLSLI

// -------------------------------------------------------------------------
// 深度フェード (Soft Particle / Depth Fade) 共通関数
// -------------------------------------------------------------------------
// 光線やバリア、パーティクルなどの半透明エフェクトが不透明オブジェクトに
// めり込んだ際の境界線を滑らかにぼかすためのユーティリティです。
// -------------------------------------------------------------------------

/**
 * 深度フェード（Soft Fade）の係数を計算します。
 * @param depthTexture 背景の深度バッファ(通常は register(t6) にバインドされる)
 * @param screenPosition 現在のピクセルのSV_Position
 * @param cameraNear カメラの近クリップ距離 (NearZ)
 * @param cameraFar カメラの遠クリップ距離 (FarZ)
 * @param softScale フェードの柔らかさ。1.0が標準。値が小さいほどグラデーションの距離が長くなる。
 * @return 0.0(完全にめり込んでいる) ～ 1.0(めり込んでいない) のアルファ係数
 */
float CalculateDepthFade(
	Texture2D<float> depthTexture, 
	float4 screenPosition, 
	float cameraNear, 
	float cameraFar, 
	float softScale = 1.0f)
{
	// 1. 画面上のピクセル座標を取得
	int3 screenPos = int3(screenPosition.xy, 0);
	
	// 2. 深度バッファから背景のZ値 (0.0 ～ 1.0) をサンプリング
	float backgroundDepthNDC = depthTexture.Load(screenPos).r;
	
	// 3. Z値を線形(ワールド空間の距離)に変換
	// LinearZ = (Near * Far) / (Far - Z * (Far - Near))
	float farMinusNear = cameraFar - cameraNear;
	float nearTimesFar = cameraNear * cameraFar;
	
	float backgroundDepthLinear = nearTimesFar / (cameraFar - backgroundDepthNDC * farMinusNear);
	float pixelDepthLinear = nearTimesFar / (cameraFar - screenPosition.z * farMinusNear);
	
	// 4. 背景との距離差を計算し、フェードさせる
	float depthDiff = backgroundDepthLinear - pixelDepthLinear;
	float fade = saturate(depthDiff * softScale);
	
	return fade;
}

#endif
