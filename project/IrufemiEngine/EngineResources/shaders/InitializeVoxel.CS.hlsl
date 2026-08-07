#include "VoxelParticle.hlsli"

StructuredBuffer<Voxel> gVoxels : register(t0);
RWStructuredBuffer<VoxelParticle> gParticles : register(u0);

[numthreads(64, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
	uint particleIndex = dispatchThreadID.x;

	uint particleCount, pStride;
	gParticles.GetDimensions(particleCount, pStride);
	if (particleIndex >= particleCount)
		return;

	// オリジナルのボクセル数を取得
	uint voxelCount, vStride;
	gVoxels.GetDimensions(voxelCount, vStride);
	
	// インスタンスごとに同じボクセルデータを参照するため剰余をとる
	uint voxelIndex = particleIndex % voxelCount;
	Voxel voxel = gVoxels[voxelIndex];
    
	gParticles[particleIndex].position = voxel.position;
	gParticles[particleIndex].velocity = float3(0.0f, 0.0f, 0.0f);
	gParticles[particleIndex].color = float4(voxel.color.rgb, 0.0f); // 初期は透明にする
	gParticles[particleIndex].life = 0.0f; // 非アクティブ
	gParticles[particleIndex].size = 1.0f;
	gParticles[particleIndex].isActive = 0; // 非アクティブ
	gParticles[particleIndex].normal = voxel.normal; // 法線をコピー
}