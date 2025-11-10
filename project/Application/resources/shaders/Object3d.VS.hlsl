
/*テクスチャを貼ろう*/

#include "./Object3d.hlsli"

/*三角形を動かそう*/

struct TransformationMatrix
{
	float32_t4x4 WVP;
	
	/*LambertianReflectance*/
	
	float32_t4x4 World;
	
	/*非均一スケール*/
	
	/// 組み込んで使う
	
	float32_t4x4 WorldInverseTranspose;
};
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

/*三角形を表示しよう*/

//struct VertexShaderOutput
//{
//	float32_t4 position : SV_POSITION;

//};

struct VertexShaderInput
{
	float32_t4 position : POSITION0;
	
	/*テクスチャを貼ろう*/
	
	///VertexShaderをtexcoord対応する
	
	float32_t2 texcoord : TEXCOORD0;
	
    /*LambertianReflectance*/
	
	float32_t3 normal : NORMAL0;
	
};

/*テクスチャを貼ろう*/

VertexShaderOutput main(VertexShaderInput input)
{
	VertexShaderOutput output;
	//output.position = input.position;
	
	/*三角形を動かそう*/
	
	output.position = mul(input.position, gTransformationMatrix.WVP);
	
	/*テクスチャを貼ろう*/
	
	///VertexShaderをtexcoord対応する
	
	output.texcoord = input.texcoord;
	
	
	///*LambertianReflectance*/
	
	/////法線の座標系を変換してPixelShaderに送る
	
	//output.normal = normalize(mul(input.normal, (float32_t3x3) gTransformationMatrix.World));
	
	/*非均一スケール*/
	
	/// 組み込んで使う
	
	// 法線を変換する際に逆転置行列を使う
	
	output.normal = normalize(mul(input.normal, (float32_t3x3) gTransformationMatrix.WorldInverseTranspose));
	
	/*PhongReflectionModel*/
	
	/// Cameraへの方向を算出
	float32_t4 worldPos = mul(input.position, gTransformationMatrix.World);
	output.worldPosition = worldPos.xyz;
	
	/*三角形を表示しよう*/

	return output;
}

