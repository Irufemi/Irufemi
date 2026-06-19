/*テクスチャを貼ろう*/

#include "ParticleGPU.hlsli"
#include "DepthFade.hlsli"

/*三角形の色を変えよう*/

#include "Material.hlsli"

ConstantBuffer<Material> gMaterial : register(b0);

struct PixelShaderOutput
{
	float32_t4 color : SV_TARGET0;
};

/*テクスチャを貼ろう*/

///Textureを使う

Texture2D<float32_t4> gTexture : register(t0); //SRVのregisterはt
Texture2D<float> gDepthTexture : register(t6); // ソフトパーティクル用深度テクスチャ

SamplerState gSamplerWrap : register(s0); //Samplerのregisterはs
SamplerState gSamplerClamp : register(s1);
SamplerState gSamplerWrapClamp : register(s4); // U:Wrap, V:Clamp

/*テクスチャを貼ろう*/

PixelShaderOutput main(VertexShaderOutput input)
{
	PixelShaderOutput output;
	
	/*UVTransform*/
	
	///Materialを拡張する
	
	float4 transformedUV = mul(float32_t4(input.texcoord.xy, 0.0f, 1.0f), gMaterial.uvTransform);
	float32_t4 textureColor = float4(1.0f, 1.0f, 1.0f, 1.0f);

	// パーティクルは基本的にラップではなくクランプを使用する
	// (境界部分での1ピクセルの回り込みによる矩形ノイズを防ぐため)
	if (gMaterial.useClampSampler == 3)
	{
		textureColor = gTexture.Sample(gSamplerWrapClamp, transformedUV.xy);
	}
	else if (gMaterial.useClampSampler == 1) // 1: Wrap を明示的に指定した場合
	{
		textureColor = gTexture.Sample(gSamplerWrap, transformedUV.xy);
	}
	else // デフォルト(0) は Clamp
	{
		textureColor = gTexture.Sample(gSamplerClamp, transformedUV.xy);
	}

	output.color = gMaterial.color * textureColor * input.color;
	
	/*2値抜き*/
		
	/// disxard
		
	// output.aolorのα値が0の時にPixelを棄却
	if (output.color.a == 0.0)
	{
		discard;
	}
	
	/*ソフトパーティクル計算*/
	
	float softScale = 1.0f; // TODO: 必要ならマテリアルやエミッターパラメータに出す
	float fade = CalculateDepthFade(gDepthTexture, input.position, input.cameraNear, input.cameraFar, softScale);
	
	// αにフェードを適用
	output.color.a *= fade;
	
	
	return output;
}