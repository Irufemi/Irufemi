#pragma once
#include "math/Transform.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Matrix4x4.h"
#include "math/shape/AABB.h"
#include "math/shape/Particle.h"

struct Particle {
	Transform transform;
	Vector3 velocity;
	Vector4 color;
	Vector3 startScale;
	Vector3 endScale;
	Vector4 startColor;
	Vector4 endColor;
	float lifeTime;
	float currentTime;

	/// <summary>
	/// パーティクルの状態を更新します。
	/// </summary>
	/// <param name="deltaTime">経過時間</param>
	void Update(float deltaTime);
};

struct ParticleForGPU
{
	Matrix4x4 WVP;
	Matrix4x4 world;
	Vector4 color;
};

struct AccelerationField {
	Vector3 acceleration; //!< 加速度
	AABB area; //!< 範囲

	void Apply(Particle& particle, float deltaTime) const;
};


// パーティクルの種類
enum class ParticleType {
	Normal,
	kAccelerationField,
	kHitEffect,
	kExplosion,
	// 他の種類をここに追加
};

// PrimitiveShape形状
enum class PrimitiveShape {
	Plane, // 板
	Sphere, // 球
	Ring, // リング
	Cylinder, // シリンダー
};