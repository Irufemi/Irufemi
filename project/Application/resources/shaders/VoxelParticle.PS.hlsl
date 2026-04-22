#include "VoxelParticle.hlsli"

struct PixelShaderOutput
{
	float4 color : SV_TARGET0;
};

// 3Dハッシュ関数（ノイズ生成用）
float Hash3D(float3 p) {
    return frac(sin(dot(p, float3(12.9898, 78.233, 45.164))) * 43758.5453);
}

PixelShaderOutput main(VertexShaderOutput input)
{
	PixelShaderOutput output;

	// Alphaチャンネルには UpdateVoxel.CS.hlsl で更新された life(1.0 -> 0.0) が入っている
	float life = input.color.a;

	if (life < 1.0f) {
		// ワールド座標ベースでブロック状の高周波ノイズを生成する（砂粒感）
		// 乗散させる係数でノイズの細かさを変える
		float noise = Hash3D(floor(input.worldPosition * 25.0f)); 

		// life が減るにつれてノイズ値が大きいピクセルから消滅する（穴があく）
		// 早めに消え始めるように life に係数をかける
		float threshold = life * 1.5f;

		if (noise > threshold) { 
			// ピクセルを描画しない（透過・侵食）
			discard;
		}
        
		// ディゾルブの溶け際（境界線）の演出
		float edge = threshold - noise;
		if (edge < 0.1f) {
			// まさに削られているフチの部分を、強烈な発光オレンジにする
			input.color.rgb += float3(3.0f, 0.8f, 0.0f);
		}
	}

	// 最終出力
	// RGBのマイナス値（炭化表現用）を0にクランプしつつ出力し、ディゾルブ用にアルファは1固定で描画
	output.color = float4(max(float3(0, 0, 0), input.color.rgb), 1.0f); 

	return output;
}