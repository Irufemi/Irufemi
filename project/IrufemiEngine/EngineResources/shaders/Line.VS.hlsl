#include "Line.hlsli"
#include "VertexData.hlsli"

#include "Transform.hlsli"
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

// struct VertexShaderInput は VertexData.hlsli で定義

VertexShaderOutput main(VertexInput input)
{
	VertexShaderOutput output;

    // WVP だけで投影
	output.position = mul(input.position, gTransformationMatrix.WVP);

    // VS は頂点カラーをそのままPSへ渡す。PS側で material カラーと乗算する。
	output.color = input.color;

	return output;
}