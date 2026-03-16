#pragma once

// C++側のVoxel構造体と一致させる
struct Voxel
{
	float3 position;
	float3 normal;
	float4 color;
	float2 uv;
};

// C++側のVoxelParticle構造体と一致させる
struct VoxelParticle
{
	float3 position;
	float life;
	float3 velocity;
	float size;
	float4 color;
	float3 normal;
	uint isActive;
};

// C++側のVoxelEmitter構造体と一致させる（合計48バイト）
struct VoxelEmitter
{
	float3 emitPosition;
	float time;
	float lifeTime;
	float gravity;
	uint emit;
	float dispersion;
	float convergence;
	float3 baseVelocity;
	float3 rotate;
	float pad1;
	float3 scale;
	float pad0;
};

// C++側のPerView構造体と一致させる
struct PerView
{
	float4x4 viewProjection;
	float4x4 billboardMatrix;
};

// 追加: 頂点シェーダー出力構造体
struct VertexShaderOutput
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
	float3 normal : NORMAL0;
	float3 worldPosition : POSITION0;
	float4 color : COLOR0;
};