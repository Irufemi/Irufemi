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
	float3 emitPosition;  // 12
	float time;           // 4  → 計16
	float lifeTime;       // 4
	float gravity;        // 4
	uint emit;            // 4
	float dispersion;     // 4  → 計32
	float convergence;    // 4
	float pad0;           // 4
	float pad1;           // 4
	float pad2;           // 4  → 計48
};

// C++側のPerView構造体と一致させる
struct PerView
{
	float4x4 viewProjection;
	float4x4 billboard;
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