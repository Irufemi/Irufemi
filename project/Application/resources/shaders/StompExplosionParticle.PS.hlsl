#include "ParticleGPU.hlsli"
#include "Material.hlsli"
#include "Noise.hlsli"

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<GPUParticleEmitter> gEmitter : register(b6);

struct PixelShaderOutput
{
	float32_t4 color : SV_TARGET0;
};

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSamplerWrap : register(s0);
SamplerState gSamplerClamp : register(s1);
SamplerState gSamplerWrapClamp : register(s4);

PixelShaderOutput main(VertexShaderOutput input)
{
	PixelShaderOutput output;
	
	float4 transformedUV = mul(float32_t4(input.texcoord.xy, 0.0f, 1.0f), gMaterial.uvTransform);
	float32_t4 textureColor = float4(1.0f, 1.0f, 1.0f, 1.0f);

	if (gMaterial.useClampSampler == 3)
	{
		textureColor = gTexture.Sample(gSamplerWrapClamp, transformedUV.xy);
	}
	else if (gMaterial.useClampSampler != 0)
	{
		textureColor = gTexture.Sample(gSamplerClamp, transformedUV.xy);
	}
	else
	{
		textureColor = gTexture.Sample(gSamplerWrap, transformedUV.xy);
	}

	float4 baseColor = gMaterial.color * textureColor * input.color;

	// ノイズのスケール（数値を大きくすると、より細かくきめ細やかなディゾルブになる）
	float noiseScale = 8.0f;
	
	// ノイズ値の生成 (ワールド座標の端数を利用して精度落ちを防ぎつつパーティクルごとにバラバラにする)
	float n = fBm(input.texcoord.xy * noiseScale + frac(input.texcoord.zw) * 10.0f);
	
	// ディゾルブ処理: 寿命が進むにつれて閾値が上がるが、最初は全く消えず、後半で一気に浸食されるように調整
	// timeRatio: 0.0 -> 1.0
	// threshold: -0.5 -> 1.0
	float threshold = input.timeRatio * 1.5f - 0.5f;
	if (n < threshold)
	{
		discard;
	}

	// エッジのハイライト表現（燃え尽きる直前の縁を光らせる）
	float edgeThickness = 0.15f;
	if (n < threshold + edgeThickness)
	{
		// 炎のような色 (RGB = 5.0, 1.5, 0.2) など、高輝度な色を乗算する
		baseColor.rgb *= float3(5.0f, 1.5f, 0.2f);
		baseColor.a = 1.0f; // エッジは不透明にする
	}

	// アルファブレンド（加算合成）で薄く消えていくのを防ぐため、最低限の不透明度を維持する
	// (timeRatioが後半になると徐々に透明になるが、エッジ部分は↑で1.0に上書きされている)
	baseColor.a = max(baseColor.a, 0.2f);

	output.color = baseColor;
	return output;
}
