#include "Particle.hlsli"
#include "Material.hlsli"

/*三角形の色を変えよう*/

ConstantBuffer<Material> gMaterial : register(b0);
#include "Bindless.hlsli"

struct PixelShaderOutput
{
	float32_t4 color : SV_TARGET0;
	float32_t4 mask  : SV_TARGET1;
	float32_t4 normal : SV_TARGET2;
	float32_t4 material : SV_TARGET3;
	float32_t2 velocity : SV_TARGET4;
};

/*テクスチャを貼ろう*/

///Textureを使う

SamplerState gSamplerWrap : register(s0); //Samplerのregisterはs
SamplerState gSamplerClamp : register(s1);

/*LambertianReflectance*/

struct DirectionalLight
{
	 //!< ライトの色
	float32_t4 color;
    //!< ライトの向き
	float32_t3 direction;
    //!< 輝度
	float intensity;
};
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);

/*テクスチャを貼ろう*/

PixelShaderOutput main(VertexShaderOutput input)
{
	PixelShaderOutput output;
	
	/*UVTransform*/
	
	///Materialを拡張する
	
	float4 transformedUV = mul(float32_t4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
	float32_t4 textureColor;
	if (gMaterial.useClampSampler != 0)
	{
		textureColor = gTextures[gMaterial.textureIndex].Sample(gSamplerClamp, transformedUV.xy);
	}
	else
	{
		textureColor = gTextures[gMaterial.textureIndex].Sample(gSamplerWrap, transformedUV.xy);
	}
	output.color = gMaterial.color * textureColor * input.color;
	
	/*2値抜き*/
		
	/// disxard
		
	// アルファテスト
	if (output.color.a == 0.0) {
		discard;
	}
	
	output.mask.r = gMaterial.customEffectType / 255.0f;
	output.mask.g = gMaterial.customEffectParam;
	output.mask.b = gMaterial.enableEffectMask ? 1.0f : 0.0f;
	output.mask.a = 1.0f;

	// パーティクルの法線はビルボードなのでZ手前固定
	output.normal = float4(0.0f, 0.0f, -1.0f, 1.0f); 
	
	// パーティクルのマテリアルは仮の値
	output.material = float4(0.0f, 1.0f, 0.0f, 1.0f);
	
	// ベロシティは仮
	output.velocity = float2(0.0f, 0.0f);

	return output;
}