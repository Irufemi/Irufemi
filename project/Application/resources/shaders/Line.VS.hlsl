#include "Line.hlsli"

struct TransformationMatrix
{
	float32_t4x4 WVP;
	float32_t4x4 World; // 未使用
	float32_t4x4 WorldInverseTranspose; // 未使用
};
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

struct VertexShaderInput
{
	float32_t4 position : POSITION0;
};

VertexShaderOutput main(VertexShaderInput input)
{
	VertexShaderOutput output;

    // WVP だけで投影
	output.position = mul(input.position, gTransformationMatrix.WVP);

    // VS はカラーを決めない。PS 側で material に基づき決定する。
	output.color = float4(1.0f, 1.0f, 1.0f, 1.0f);

	return output;
}