#include "Object3d.hlsli"

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b6);

struct VertexShaderInput
{
	float32_t4 position : POSITION0;
	float32_t2 texcoord : TEXCOORD0;
	float32_t3 normal : NORMAL0;
};

VertexShaderOutput main(VertexShaderInput input)
{
	VertexShaderOutput output;

	// GSで座標変換するため、ここではワールド座標をそのままpositionに格納して渡す
	output.position = mul(input.position, gTransformationMatrix.World);
	
	output.texcoord = input.texcoord;
	
	output.normal = normalize(mul(input.normal, (float32_t3x3) gTransformationMatrix.WorldInverseTranspose));
	
	// worldPositionにもワールド座標を格納
	output.worldPosition = output.position.xyz;

	return output;
}