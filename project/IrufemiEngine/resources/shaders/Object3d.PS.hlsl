
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
	float32_t3 worldPosition;
};
ConstantBuffer<Camera> gCamera : register(b2);

/*PointLight*/

struct PointLight
{
	//!< ライトの色
	float32_t4 color;
	//!< ライトの位置
	float32_t3 position;
	//!< 輝度
	float intensity;
	
};
ConstantBuffer<PointLight> gPointLight : register(b3);

/*SpotLight*/
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
};
ConstantBuffer<SpotLight> gSpotLight : register(b4);

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
			
			/*PointLight*/
			
			/// 入射光を計算する
			
			// 物体表面の特定の点に対する入射光を計算する
			float32_t3 pointLightDirection = normalize(input.worldPosition - gPointLight.position);
			
			// 拡散（Directional と同じモードで計算）
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
			
			// 鏡面（Blinn-Phong）
			float32_t3 halfVectorPoint = normalize(-pointLightDirection + toEye);
			float NDotHPoint = dot(normalize(input.normal), halfVectorPoint);
			float specularPowPoint = pow(saturate(NDotHPoint), gMaterial.shininess);
			// Point 拡散・鏡面
			float32_t3 diffusePoint = gMaterial.color.rgb * textureColor.rgb * gPointLight.color.rgb * cosPoint * gPointLight.intensity;
			float32_t3 specularPoint = gPointLight.color.rgb * gPointLight.intensity * specularPowPoint * float32_t3(1.0f, 1.0f, 1.0f);
			
			/*SpotLight*/

            /// 入射光（ライト→表面の向き）
			float32_t3 spotLightDirectionOnSurface = normalize(input.worldPosition - gSpotLight.position);

            // 距離減衰（0..1）：distance と decay を使用
			float d = length(input.worldPosition - gSpotLight.position);
			float attenuationFactor = pow(saturate(1.0f - d / max(gSpotLight.distance, 1e-5f)), gSpotLight.decay);

            // 角度減衰（Falloff）：中心1、閾値 cosAngle で0
			float cosAngleSpot = dot(spotLightDirectionOnSurface, gSpotLight.direction); // 両方とも単位ベクトル前提
			float falloffFactor = saturate((cosAngleSpot - gSpotLight.cosAngle) / (1.0f - gSpotLight.cosAngle));

            // 拡散（Lambert/Half-Lambert は Directional/Point と同じ分岐）
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

            // 鏡面（Blinn-Phong）
			float32_t3 halfVectorSpot = normalize(-spotLightDirectionOnSurface + toEye);
			float NDotHSpot = dot(normalize(input.normal), halfVectorSpot);
			float specularPowSpot = pow(saturate(NDotHSpot), gMaterial.shininess);

            // Spot 拡散・鏡面
			float32_t3 diffuseSpot =
                gMaterial.color.rgb * textureColor.rgb * gSpotLight.color.rgb *
                cosSpot * gSpotLight.intensity * attenuationFactor * falloffFactor;

			float32_t3 specularSpot =
                gSpotLight.color.rgb * gSpotLight.intensity *
                specularPowSpot * attenuationFactor * falloffFactor * float32_t3(1.0f, 1.0f, 1.0f);

			/*PointLight*/	
			
			/// 全部足す
			
			// 最終的な色はどのように決まるのかといえば、DirectionalLightとPointLightでそれぞれ計算したDiffuse/Specularをすべて足し合わせて求める
			output.color.rgb = diffuse + specular + diffusePoint + specularPoint + diffuseSpot + specularSpot;
			
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
	
	return output;
}


