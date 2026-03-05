/*テクスチャを貼ろう*/

#include "ParticleGPU.hlsli"

/*三角形の色を変えよう*/

struct ParticleMaterial
{
	float32_t4 color;
	int32_t useClampSampler; // 0: WRAP, 1: CLAMP
	float3 _padding;
	float32_t4x4 uvTransform;
};
ConstantBuffer<ParticleMaterial> gMaterial : register(b0);

struct PixelShaderOutput
{
	float32_t4 color : SV_TARGET0;
};

/*テクスチャを貼ろう*/

///Textureを使う

Texture2D<float32_t4> gTexture : register(t0); //SRVのregisterはt
SamplerState gSamplerWrap : register(s0); //Samplerのregisterはs
SamplerState gSamplerClamp : register(s1);

/*テクスチャを貼ろう*/

PixelShaderOutput main(VertexShaderOutput input)
{
	PixelShaderOutput output;
	
	/*UVTransform*/
	
	///Materialを拡張する
	
	float4 transformedUV = mul(float32_t4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
	float32_t4 textureColor = float4(1.0f, 1.0f, 1.0f, 1.0f);

	if (gMaterial.useClampSampler != 0)
	{
		textureColor = gTexture.Sample(gSamplerClamp, transformedUV.xy);
	}
	else
	{
		textureColor = gTexture.Sample(gSamplerWrap, transformedUV.xy);
	}

	output.color = gMaterial.color * textureColor * input.color;
	
	/*2値抜き*/
		
	/// disxard
		
	// output.aolorのα値が0の時にPixelを棄却
	if (output.color.a == 0.0)
	{
		discard;
	}
	
	
	return output;
}