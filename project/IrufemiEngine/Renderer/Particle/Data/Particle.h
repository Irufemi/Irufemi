#pragma once
#include "../../../Engine/Core/Math/Transform.h"
#include "../../../Engine/Core/Math/Vector3.h"
#include "../../../Engine/Core/Math/Vector4.h"
#include "../../../Engine/Core/Math/Matrix4x4.h"
#include "../../../Engine/Core/Math/Geometry/AABB.h"


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
	kMuzzleSmoke, // 排莢口の煙
	kMuzzleFlash, // 銃口の火花
	kMissileFire, // ミサイルの炎
	kMissileSmoke, // ミサイルの煙
	// 他の種類をここに追加
};