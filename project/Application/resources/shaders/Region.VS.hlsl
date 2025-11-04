// Blocks 用インスタンシング VS（Particle と同様に VS: t0 を使用）
// RootParameter[4] (VS) に SRV テーブルをバインド（t0）
// 出力は Object3d.hlsli の VertexShaderOutput に合わせる

#include "./Object3d.hlsli"

struct InstanceData
{
	float32_t4x4 WVP;
	float32_t4x4 World;
	float32_t4x4 WorldInverseTranspose;
	float32_t4 color; // 未使用なら無視
};
StructuredBuffer<InstanceData> gBlocks : register(t0);

struct VertexShaderInput
{
	float32_t4 position : POSITION0;
	float32_t2 texcoord : TEXCOORD0;
	float32_t3 normal : NORMAL0;
};

VertexShaderOutput main(VertexShaderInput input, uint32_t instanceId : SV_InstanceID)
{
	VertexShaderOutput output;

	InstanceData inst = gBlocks[instanceId];

    // 位置
	output.position = mul(input.position, inst.WVP);

    // UV
	output.texcoord = input.texcoord;

    // 法線（逆転置行列で変換）
	float32_t4 n4 = mul(float32_t4(input.normal, 0.0f), inst.WorldInverseTranspose);
	output.normal = normalize(n4.xyz);

    // ワールド座標（PS 側で視線方向などに使用）
	float32_t4 worldPos = mul(input.position, inst.World);
	output.worldPosition = worldPos.xyz;

	return output;
}