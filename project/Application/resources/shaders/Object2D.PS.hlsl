// - αカットアウト(閾値0.5)のみ
// - ライティング関連は削除

#include "./Object2D.hlsli" // 2D用の入出力定義へ切替

struct Material
{
	float32_t4 color;

    // 以下はCPU側との互換性のため残置（本PSでは未使用）
	int32_t enableLighting; // 未使用
	int32_t hasTexture; // 使用
	int32_t lightingMode; // 未使用
	float padding; // 未使用

    // UV変換は使用
	float32_t4x4 uvTransform;
};
ConstantBuffer<Material> gMaterial : register(b0);

struct PixelShaderOutput
{
	float32_t4 color : SV_TARGET0;
};

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

PixelShaderOutput main(VertexShaderOutput input)
{
	PixelShaderOutput output;

    // UV 変換
	float32_t4 uvw = mul(float32_t4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
	float32_t2 uv = uvw.xy;

    // hasTexture 時のみサンプル（帯域節約）
	float32_t4 texColor = (gMaterial.hasTexture != 0)
        ? gTexture.Sample(gSampler, uv)
        : float32_t4(1.0f, 1.0f, 1.0f, 1.0f);

    // ベースカラー
	float32_t4 baseColor = texColor * gMaterial.color;

    // αカットアウト：テクスチャ使用時のみ（閾値0.5）
    // → 無地半透明（hasTexture == 0）の場合は discard しない（加算/通常ブレンドが効く）
	if (gMaterial.hasTexture != 0 && baseColor.a <= 0.5f)
	{
		discard;
	}

	output.color = baseColor;
	return output;
}