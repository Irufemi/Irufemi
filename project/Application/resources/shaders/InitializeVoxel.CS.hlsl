#include "VoxelParticle.hlsli"
#include "RandomGenerator.hlsli"
#include "PerFrame.hlsli"

StructuredBuffer<Voxel> gVoxels : register(t0);
RWStructuredBuffer<VoxelParticle> gParticles : register(u0);
ConstantBuffer<VoxelEmitter> gEmitter : register(b0);
ConstantBuffer<PerFrame> gPerFrame : register(b1);

[numthreads(64, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
	uint voxelIndex = dispatchThreadID.x;

    // エミットトリガーが押されたフレームでのみ初期化
	if (gEmitter.emit == 0)
	{
        // トリガーがない場合は非アクティブ化
		if (gParticles[voxelIndex].isActive == 1)
		{
			gParticles[voxelIndex].isActive = 0;
		}
		return;
	}

	Voxel voxel = gVoxels[voxelIndex];
    
	RandomGenerator generator;
    // 乱数のシードを時間とインデックスで初期化
	generator.seed = voxelIndex * 12345 + uint3(1, 7, 11) * (uint) (gPerFrame.time * 1000.0f);

    // 初期位置: エミッター位置 + ボクセルの相対位置
	gParticles[voxelIndex].position = gEmitter.emitPosition + voxel.position;
    
    // 初期速度: ボクセルの法線方向をベースにランダム性を加える
	float3 randomVec = (generator.Generate3d() * 2.0f - 1.0f) * 0.5f; // -0.5 ~ 0.5
	gParticles[voxelIndex].velocity = normalize(voxel.normal + randomVec) * gEmitter.dispersion;

	gParticles[voxelIndex].color = voxel.color;
	gParticles[voxelIndex].life = 1.0f; // 生存時間(1.0で開始)
	gParticles[voxelIndex].size = 1.0f; // サイズ
	gParticles[voxelIndex].isActive = 1; // アクティブ化
}