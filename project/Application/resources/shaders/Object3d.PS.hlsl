/*テクスチャを貼ろう*/

#include "./Object3d.hlsli"

/*三角形の色を変えよう*/

struct Material
{
	float32_t4 color;
	
	/*LambertianReflectance*/
	
	int32_t enableLighting;
	
	int32_t hasTexture;
	
	 // 0=Lightingなし, 1=Lambert, 2=HalfLambert
	int32_t lightingMode;
	
	float padding;
	
	/*UVTransform*/
	
	///Materialの拡張
	
	float32_t4x4 uvTransform;

	float32_t shininess;

    // 環境マップの映り込み係数
	float32_t environmentCoefficient;

	float32_t2 padding2; // パディング
	
};
ConstantBuffer<Material> gMaterial : register(b0);
struct PixelShaderOutput
{
	float32_t4 color : SV_TARGET0;
};

/*テクスチャを貼ろう*/

///Textureを使う

Texture2D<float32_t4> gTexture : register(t0); //SRVのregisterはt
SamplerState gSampler : register(s0); //Samplerのregisterはs

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

/*PhongReflectionModel*/

/// カメラの位置を送る

struct Camera
{
	float32_t4x4 view;
	float32_t4x4 projection;
	float32_t3 worldPosition;
};
ConstantBuffer<Camera> gCamera : register(b2);

/*PointLight*/

#define MAX_POINT_LIGHTS 4
struct PointLight
{
	//!< ライトの色
	float32_t4 color;
	//!< ライトの位置
	float32_t3 position;
	//!< 輝度
	float intensity;
	//!< ライトの影響範囲
	float radius;
	//!< 減衰率
	float decay;
    //!< 有効フラグ
	int32_t isActive;
	float padding;
};
struct PointLights
{
	PointLight lights[MAX_POINT_LIGHTS];
};
ConstantBuffer<PointLights> gPointLights : register(b3);


/*SpotLight*/
#define MAX_SPOT_LIGHTS 4
struct SpotLight
{
	//!< ライトの色
	float32_t4 color;
	//!< ライトの位置
	float32_t3 position;
	//!< 輝度
	float32_t intensity;
	//!< スポットライトの方向
	float32_t3 direction;
	//!< ライトの届く最大距離
	float32_t distance;
	//!< 減衰率
	float32_t decay;
	//!< スポットライトの余弦
	float32_t cosAngle;
	//!< フォールオフ
	float32_t falloff;
    //!< 有効フラグ
	int32_t isActive;
	float32_t3 padding;
};
struct SpotLights
{
	SpotLight lights[MAX_SPOT_LIGHTS];
};
ConstantBuffer<SpotLights> gSpotLights : register(b4);

#define MAX_AREA_LIGHTS 4
struct AreaLight
{
    //!< ライトの色
	float4 color;
    //!< ライトの位置
	float3 position;
    //!< 輝度
	float intensity;
    //!< スポットライトの方向
	float3 direction;
    //!< ライトの届く最大距離
	float range;
    //!< 矩形のサイズ(幅、高さ)
	float2 size;
    //!< 有効フラグ
	int32_t isActive;
	float padding;
};
struct AreaLights
{
	AreaLight lights[MAX_AREA_LIGHTS];
};
ConstantBuffer<AreaLights> gAreaLights : register(b7);

/*周囲の映り込み*/

/// 環境マップを追加する

TextureCube<float32_t4> gEnviromentTexture : register(t1);

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
	
	/*テクスチャを貼ろう*/
	
	///Lightingの計算を行う
	
	if (gMaterial.enableLighting != 0) //Lightingする場合
	{
	
		float cos = 1.0f;
	
		if (gMaterial.lightingMode == 0)
		{
			if (gMaterial.hasTexture == 1)
			{
				output.color = gMaterial.color * textureColor;
			}
			else
			{
				output.color = gMaterial.color;
				output.color.a = 1.0f;
			}
		}
		else
		{
			if (gMaterial.lightingMode == 1)
			{
				cos = saturate(dot(normalize(input.normal), -gDirectionalLight.direction));
				output.color = gMaterial.color * textureColor * gDirectionalLight.color * cos * gDirectionalLight.intensity;
			}
			else if (gMaterial.lightingMode == 2)
			{
		
				/*HalfLambert*/
		
				///HalfLambertを実装する
		
				//HalfLambert
				float NdotL = dot(normalize(input.normal), -gDirectionalLight.direction);
				cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
		
			}

			/*PhongReflectionModel*/

			/// カメラの位置を送る
	
			// Cameraへの方向を算出
			float32_t3 toEye = normalize(gCamera.worldPosition - input.worldPosition);

			/// 反射ベクトルと強度を求める
	
			// 入射光の反射ベクトルを求める
			float32_t3 reflectRight = reflect(gDirectionalLight.direction, normalize(input.normal));
	
			//// 内積をとる
			//float RdotE = dot(reflectRight, toEye);
			//// 鏡面反射の強度を求める
			//float specularPow = pow(saturate(RdotE), gMaterial.shininess);
			
			/*BlinnPhongReflectionModel*/
			
			/// HalfVectorを求めて計算する
			
			float32_t3 halfVector = normalize(-gDirectionalLight.direction + toEye);
			float NDotH = dot(normalize(input.normal), halfVector);
			float specularPow = pow(saturate(NDotH), gMaterial.shininess);

			/*PhongReflectionModel*/
	
			/// すべてを1つに
			// 拡散反射
			float32_t3 diffuse = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
			// 鏡面反射
			float32_t3 specular = gDirectionalLight.color.rgb * gDirectionalLight.intensity * specularPow * float32_t3(1.0f, 1.0f, 1.0f);
			
			float32_t3 totalDiffuse = diffuse;
			float32_t3 totalSpecular = specular;
			
			/*PointLight*/
			for (int i = 0; i < MAX_POINT_LIGHTS; ++i)
			{
				if (gPointLights.lights[i].isActive == 0)
				{
					continue;
				}
				PointLight light = gPointLights.lights[i];
				
				/// 入射光を計算する
				// 物体表面の特定の点に対する入射光を計算する
				float32_t3 pointLightDirection = normalize(input.worldPosition - light.position);
			
				// 拡散(Directional と同じモードで計算)
				float cosPoint = 1.0f;
				if (gMaterial.lightingMode == 1)
				{
					cosPoint = saturate(dot(normalize(input.normal), -pointLightDirection));
				}
				else if (gMaterial.lightingMode == 2)
				{
					float NdotLPoint = dot(normalize(input.normal), -pointLightDirection);
					cosPoint = pow(NdotLPoint * 0.5f + 0.5f, 2.0f);
				}
			
				// 鏡面(Blinn-Phong)
				float32_t3 halfVectorPoint = normalize(-pointLightDirection + toEye);
				float NDotHPoint = dot(normalize(input.normal), halfVectorPoint);
				float specularPowPoint = pow(saturate(NDotHPoint), gMaterial.shininess);
				// Point 拡散・鏡面
				float32_t3 diffusePoint = gMaterial.color.rgb * textureColor.rgb * light.color.rgb * cosPoint * light.intensity;
				float32_t3 specularPoint = light.color.rgb * light.intensity * specularPowPoint * float32_t3(1.0f, 1.0f, 1.0f);
				
				totalDiffuse += diffusePoint;
				totalSpecular += specularPoint;
			}
			
			/*SpotLight*/
			for (int i = 0; i < MAX_SPOT_LIGHTS; ++i)
			{
				if (gSpotLights.lights[i].isActive == 0)
				{
					continue;
				}
				SpotLight light = gSpotLights.lights[i];

				/// 入射光(ライト→表面の向き)
				float32_t3 spotLightDirectionOnSurface = normalize(input.worldPosition - light.position);

				// 距離減衰(0..1)：distance と decay を使用
				float d = length(input.worldPosition - light.position);
				float attenuationFactor = pow(saturate(1.0f - d / max(light.distance, 1e-5f)), light.decay);

				// 角度減衰(Falloff)：中心1、閾値 cosAngle で0
				float cosAngleSpot = dot(spotLightDirectionOnSurface, light.direction); // 両方とも単位ベクトル前提
				float falloffFactor = saturate((cosAngleSpot - light.cosAngle) / (1.0f - light.cosAngle));

				// 拡散(Lambert/Half-Lambert は Directional/Point と同じ分岐)
				float cosSpot = 1.0f;
				if (gMaterial.lightingMode == 1)
				{
					cosSpot = saturate(dot(normalize(input.normal), -spotLightDirectionOnSurface));
				}
				else if (gMaterial.lightingMode == 2)
				{
					float NdotLSpot = dot(normalize(input.normal), -spotLightDirectionOnSurface);
					cosSpot = pow(NdotLSpot * 0.5f + 0.5f, 2.0f);
				}

				// 鏡面(Blinn-Phong)
				float32_t3 halfVectorSpot = normalize(-spotLightDirectionOnSurface + toEye);
				float NDotHSpot = dot(normalize(input.normal), halfVectorSpot);
				float specularPowSpot = pow(saturate(NDotHSpot), gMaterial.shininess);

				// Spot 拡散・鏡面
				float32_t3 diffuseSpot =
					gMaterial.color.rgb * textureColor.rgb * light.color.rgb *
					cosSpot * light.intensity * attenuationFactor * falloffFactor;

				float32_t3 specularSpot =
					light.color.rgb * light.intensity *
					specularPowSpot * attenuationFactor * falloffFactor * float32_t3(1.0f, 1.0f, 1.0f);

				totalDiffuse += diffuseSpot;
				totalSpecular += specularSpot;
			}

			/*AreaLight*/
			for (int i = 0; i < MAX_AREA_LIGHTS; ++i)
			{
				if (gAreaLights.lights[i].isActive == 0)
				{
					continue;
				}
				AreaLight light = gAreaLights.lights[i];

                // 距離減衰
				float d = length(input.worldPosition - light.position);
				float attenuation = pow(saturate(1.0f - d / max(light.range, 1e-5f)), 1.0f);

                // ライトの向きと法線のなす角
				float cosAngle = dot(normalize(input.normal), -light.direction);

				float diffuseFactor = 0.0f;
				if (gMaterial.lightingMode == 1) // Lambert
				{
					diffuseFactor = saturate(cosAngle);
				}
				else if (gMaterial.lightingMode == 2) // Half-Lambert
				{
					diffuseFactor = pow(cosAngle * 0.5f + 0.5f, 2.0f);
				}

				float32_t3 diffuseArea =
					gMaterial.color.rgb * textureColor.rgb * light.color.rgb *
					diffuseFactor * light.intensity * attenuation;

				totalDiffuse += diffuseArea;

				// 鏡面反射(Blinn-Phong)
				float32_t3 halfVectorArea = normalize(-light.direction + toEye);
				float NDotHArea = dot(normalize(input.normal), halfVectorArea);
				float specularPowArea = pow(saturate(NDotHArea), gMaterial.shininess);
				float32_t3 specularArea = light.color.rgb * light.intensity * specularPowArea * attenuation * float32_t3(1.0f, 1.0f, 1.0f);

				totalSpecular += specularArea;
			}


			/*PointLight*/	
			
			/// 全部足す
			
			// 最終的な色はどのように決まるのかといえば、DirectionalLightとPointLightでそれぞれ計算したDiffuse/Specularをすべて足し合わせて求める
			output.color.rgb = totalDiffuse + totalSpecular;
			
			if (gMaterial.hasTexture == 1)
			{
				// アルファは今まで通り
				output.color.a = gMaterial.color.a * textureColor.a;
			}
			else
			{
				output.color.a = 1.0f;
			}
		}
		
		/*2値抜き*/
		
		/// disxard
		
		// output.aolorのα値が0の時にPixelを棄却
		if (output.color.a == 0.0)
		{
			discard;
		}
	}
	else
	{
	
		if (gMaterial.hasTexture == 1)
		{
			output.color = gMaterial.color * textureColor;
		}
		else
		{
			output.color = gMaterial.color;
			output.color.a = 1.0f;
		}
		
	}
	
	/*周囲の映り込み*/
	
	/// 環境マップを追加する
	
	if (gMaterial.enableLighting != 0)
	{
		float32_t3 cameraToPosition = normalize(input.worldPosition - gCamera.worldPosition);
		float32_t3 reflectedVector = reflect(cameraToPosition, normalize(input.normal));
		float32_t4 enviromentColor = gEnviromentTexture.Sample(gSampler, reflectedVector);
	
		output.color.rgb += enviromentColor.rgb * gMaterial.environmentCoefficient;
	}
	
	return output;
}