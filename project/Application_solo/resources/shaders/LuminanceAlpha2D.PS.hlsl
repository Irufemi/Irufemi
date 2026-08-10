#include "./Object2D.hlsli"
#include "Material.hlsli"

ConstantBuffer<Material> gMaterial : register(b0);

#include "Bindless.hlsli"



SamplerState gSampler : register(s0);

PixelShaderOutput main(VertexShaderOutput input)
{
	PixelShaderOutput output;

    // UV 変換
	float32_t4 uvw = mul(float32_t4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
	float32_t2 uv = uvw.xy;

    // hasTexture 時のみサンプル
	float32_t4 texColor = (gMaterial.hasTexture != 0)
        ? gTextures[gMaterial.textureIndex].Sample(gSampler, uv)
        : float32_t4(1.0f, 1.0f, 1.0f, 1.0f);

    // ベースカラー計算
	float32_t4 baseColor = texColor * gMaterial.color * input.color;

    // 【輝度をアルファに変換】
    // 黒背景を完全に透過させるため、RGBの輝度(Luminance)を計算してアルファ値に掛ける
    float luminance = dot(baseColor.rgb, float3(0.299f, 0.587f, 0.114f));
    baseColor.a *= luminance;
    
    // 【HDRエミッシブブースト】
    // Bloomパスで自発光しているように見せるため、色をブーストする。
    // 背景の明るさに負けないように輝度を3.0倍以上に高める。
    float emissiveIntensity = 3.0f; 
    baseColor.rgb *= emissiveIntensity;

	output.color = baseColor;
	return output;
}
