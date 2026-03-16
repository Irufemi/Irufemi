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

	// バッファ範囲チェック
	uint count, stride;
	gVoxels.GetDimensions(count, stride);
	if (voxelIndex >= count)
		return;

	Voxel voxel = gVoxels[voxelIndex];
    
	RandomGenerator generator;
    // 乱数のシードを時間とインデックスで初期化
	generator.seed = voxelIndex * 12345 + uint3(1, 7, 11) * (uint) (gPerFrame.time * 1000.0f);

    // 1. スケーリング
	float3 localPos = voxel.position * gEmitter.scale;
    
    // 2. 回転 (XYZの順で回転行列を作成して適用)
	float cX = cos(gEmitter.rotate.x);
	float sX = sin(gEmitter.rotate.x);
	float cY = cos(gEmitter.rotate.y);
	float sY = sin(gEmitter.rotate.y);
	float cZ = cos(gEmitter.rotate.z);
	float sZ = sin(gEmitter.rotate.z);

	float3x3 rotX = { 1, 0, 0, 0, cX, -sX, 0, sX, cX };
	float3x3 rotY = { cY, 0, sY, 0, 1, 0, -sY, 0, cY };
	float3x3 rotZ = { cZ, -sZ, 0, sZ, cZ, 0, 0, 0, 1 };
	float3x3 rotateMat = mul(rotZ, mul(rotY, rotX));
    
	localPos = mul(rotateMat, localPos);
    
    // 3. 初期位置: エミッター位置 + 回転・スケール適用後のボクセル相対位置
	gParticles[voxelIndex].position = gEmitter.emitPosition + localPos;
    
    // 4. 初期速度: ボクセルの法線を同様に回転させ、ランダム性とベース速度を加える
	float3 rotatedNormal = normalize(mul(rotateMat, voxel.normal));
	float3 randomVec = (generator.Generate3d() * 2.0f - 1.0f) * 0.5f; // -0.5 ~ 0.5
	gParticles[voxelIndex].velocity = gEmitter.baseVelocity + normalize(rotatedNormal + randomVec) * gEmitter.dispersion;

	gParticles[voxelIndex].color = voxel.color;
	gParticles[voxelIndex].life = 1.0f; // 寿命を満タンにする
	gParticles[voxelIndex].size = 1.0f; // サイズ
	gParticles[voxelIndex].isActive = 1; // アクティブ化
	gParticles[voxelIndex].normal = rotatedNormal; // 回転後の法線をコピー
}