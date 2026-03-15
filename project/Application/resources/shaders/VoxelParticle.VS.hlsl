#include "VoxelParticle.hlsli"

// 頂点シェーダーの入力
struct VSInput
{
	float4 position : POSITION0;
	float2 texcoord : TEXCOORD0;
	float3 normal : NORMAL0;
};

// パーティクルごとのデータ
StructuredBuffer<VoxelParticle> gParticles : register(t0);
ConstantBuffer<PerView> gPerView : register(b0);

VertexShaderOutput main(VSInput input, uint instanceID : SV_InstanceID)
{
	VertexShaderOutput output;
	VoxelParticle particle = gParticles[instanceID];

	// 削除した (死んだパーティクルは PS の color.a <= 0 で discard される)
	// 初期状態(isActive==0)でも、元の形状を描画する必要があるためカリングしない。

    // ワールド行列を作成
    // ここでは単純な平行移動のみ
	float4x4 worldMatrix =
	{
		1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        particle.position.x, particle.position.y, particle.position.z, 1
	};

    // 位置変換
	float4 worldPos = mul(input.position, worldMatrix);
	output.position = mul(worldPos, gPerView.viewProjection);
	output.worldPosition = worldPos.xyz;

    // 法線変換（パーティクル固有の法線を渡す）
	output.normal = particle.normal;
    
    // UVと色
	output.texcoord = input.texcoord;
	output.color = particle.color;

	return output;
}