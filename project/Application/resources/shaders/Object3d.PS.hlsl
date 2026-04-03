/*テクスチャを貼ろう*/

#include "./Object3d.hlsli"
#include "./Lighting.hlsli"

/*三角形の色を変えよう*/

ConstantBuffer<Material> gMaterial : register(b0);
struct PixelShaderOutput
{
	float32_t4 color : SV_TARGET0;
};

/*テクスチャを貼ろう*/

///Textureを使う

Texture2D<float32_t4> gTexture : register(t0); //SRVのregisterはt
SamplerState gSampler : register(s0); //Samplerのregisterはs

/*Light Common & DirectionalLight*/

ConstantBuffer<LightCommonData> gLightCommon : register(b1);

/*PhongReflectionModel*/

/// カメラの位置を送る

struct Camera
{
	float32_t4x4 view;
	float32_t4x4 projection;
	float32_t3 worldPosition;
};
ConstantBuffer<Camera> gCamera : register(b2);

/*Structured Light Buffers*/

StructuredBuffer<PointLight> gPointLights : register(t2);
StructuredBuffer<SpotLight> gSpotLights : register(t3);
StructuredBuffer<AreaLight> gAreaLights : register(t4);

/*周囲の映り込み*/

/// 環境マップを追加する

//TextureCube<float32_t4> gEnviromentTexture : register(t1);

/*テクスチャを貼ろう*/

PixelShaderOutput main(VertexShaderOutput input)
{
	PixelShaderOutput output;
	
	/*UVTransform*/
	
	///Materialを拡張する
	
	float4 transformedUV = mul(float32_t4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
	float32_t4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
	
	/*2値抜き*/
		
	/// disxard
		
	// textureのα値が0.5以下の時にPixelを棄却
	if (textureColor.a <= 0.5)
	{
		discard;
	}
		
	// textureのα値が0の時にPixelを棄却
	if (textureColor.a == 0.0)
	{
		discard;
	}
	
	///Lightingの計算を行う
	
	if (gMaterial.enableLighting != 0) //Lightingする場合
	{
	
		if (gMaterial.lightingMode == 0)
		{
			output.color = gMaterial.color * textureColor;
		}
		else
		{
			LightContext context;
			context.normal = normalize(input.normal);
			context.worldPosition = input.worldPosition;
			context.toEye = normalize(gCamera.worldPosition - input.worldPosition);

			float3 totalDiffuse = 0;
			float3 totalSpecular = 0;

			// 平行光源
			ApplyDirectionalLight(gLightCommon.directionalLight, gMaterial, context, totalDiffuse, totalSpecular);

			// 点光源
			for (uint32_t i = 0; i < gLightCommon.pointLightCount; ++i) {
				ApplyPointLight(gPointLights[i], gMaterial, context, totalDiffuse, totalSpecular);
			}

			// スポットライト
			for (uint32_t j = 0; j < gLightCommon.spotLightCount; ++j) {
				ApplySpotLight(gSpotLights[j], gMaterial, context, totalDiffuse, totalSpecular);
			}

			// エリアライト
			for (uint32_t k = 0; k < gLightCommon.areaLightCount; ++k) {
				ApplyAreaLight(gAreaLights[k], gMaterial, context, totalDiffuse, totalSpecular);
			}

			// 拡散反射・鏡面反射の合成
			output.color.rgb = (totalDiffuse * gMaterial.color.rgb * textureColor.rgb) + totalSpecular;
			
			// アルファ
			output.color.a = gMaterial.color.a * textureColor.a;
		}
		
		/*2値抜き*/
		
		/// disxard
		
		// output.colorのα値が0の時にPixelを棄却
		if (output.color.a == 0.0)
		{
			discard;
		}
	}
	else
	{
	
		output.color = gMaterial.color * textureColor;
		
	}
	
	///*周囲の映り込み*/
	
	///// 環境マップを追加する
	
	//if (gMaterial.enableLighting != 0)
	//{
	//	float32_t3 cameraToPosition = normalize(input.worldPosition - gCamera.worldPosition);
	//	float32_t3 reflectedVector = reflect(cameraToPosition, normalize(input.normal));
	//	float32_t4 enviromentColor = gEnviromentTexture.Sample(gSampler, reflectedVector);
	
	//	output.color.rgb += enviromentColor.rgb * gMaterial.environmentCoefficient;
	//}
	
	return output;
}