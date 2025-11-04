// Sprite 用最小 VS（CPU側のCBレイアウトはWVP+Worldのまま維持。Worldは未使用）

#include "./Object2D.hlsli"

struct TransformationMatrix
{
	float32_t4x4 WVP;
	float32_t4x4 World; // レイアウト維持のため残す（未使用）
};
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

struct VertexShaderInput
{
	float32_t4 position : POSITION0;
	float32_t2 texcoord : TEXCOORD0;
	float32_t3 normal : NORMAL0; // 入力は維持（未使用でも可）
};

VertexShaderOutput main(VertexShaderInput input)
{
	VertexShaderOutput output;

    // WVP だけで投影
	output.position = mul(input.position, gTransformationMatrix.WVP);

    // UV はそのまま
	output.texcoord = input.texcoord;

    // Unlit 想定なので法線は固定（PSでは未使用）
	output.normal = float32_t3(0.0f, 0.0f, -1.0f);

	return output;
}