/*テクスチャを貼ろう*/

#include "ParticleGPU.hlsli"

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

	output.color = gMaterial.color * textureColor * input.color;
	
	/*2値抜き*/
		
	/// disxard
		
	// output.aolorのα値が0の時にPixelを棄却
	if (output.color.a == 0.0)
	{
		discard;
	}
	
	/*ソフトパーティクル計算*/
	
	// 1. 画面上のピクセル座標を取得
	int3 screenPos = int3(input.position.xy, 0);
	
	// 2. 深度バッファから背景のZ値 (0.0 ～ 1.0) をサンプリング
	float backgroundDepthNDC = gDepthTexture.Load(screenPos).r;
	
	// 3. Z値を線形(ワールド空間の距離)に変換
	// LinearZ = (Near * Far) / (Far - Z * (Far - Near))
	float farMinusNear = input.cameraFar - input.cameraNear;
	float nearTimesFar = input.cameraNear * input.cameraFar;
	
	float backgroundDepthLinear = nearTimesFar / (input.cameraFar - backgroundDepthNDC * farMinusNear);
	float particleDepthLinear = nearTimesFar / (input.cameraFar - input.position.z * farMinusNear);
	
	// 4. 背景との距離差を計算し、フェードさせる (1.0f は SoftnessScale 調整用)
	float depthDiff = backgroundDepthLinear - particleDepthLinear;
	float softScale = 1.0f; // TODO: 必要ならマテリアルやエミッターパラメータに出す
	float fade = saturate(depthDiff * softScale);
	
	// αにフェードを適用
	output.color.a *= fade;
	
	
	return output;
}