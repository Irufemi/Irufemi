#include "VoxelParticle.hlsli"
#include "VertexData.hlsli"

// struct VSInput は VertexData.hlsli (VertexInput) で定義

// パーティクルごとのデータ
StructuredBuffer<VoxelParticle> gParticles : register(t1);
ConstantBuffer<PerView> gPerView : register(b6);
ConstantBuffer<VoxelEmitter> gEmitter : register(b0);

VertexShaderOutput main(VertexInput input, uint instanceID : SV_InstanceID)
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

    // 位置変換 (モデルスケールとパーティクルサイズを適用)
	float4 localPos = input.position;
	localPos.xyz *= gEmitter.scale * particle.size; 
	float4 worldPos = mul(localPos, worldMatrix);
    
    // 非アクティブなら画面外へ飛ばす
    if (particle.isActive == 0) {
        worldPos.xyz = float3(0, -10000, 0);
    }
    
	output.position = mul(worldPos, gPerView.viewProjection);
	output.worldPosition = worldPos.xyz;

    // 法線変換（パーティクル固有の法線を渡す）
	output.normal = particle.normal;
    
    // UVと色
	output.texcoord = input.texcoord;
	output.color = input.color * particle.color;

	return output;
}