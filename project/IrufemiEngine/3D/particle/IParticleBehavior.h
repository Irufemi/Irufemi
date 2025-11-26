#pragma once

#include "math/shape/Particle.h"
#include "math/Emitter.h"
#include <random>
#include <memory>

// 前方宣言
class DebugUI;

/// <summary>
/// パーティクルの振る舞いを定義するインターフェース
/// </summary>
class IParticleBehavior {
public:
	virtual ~IParticleBehavior() = default;

	/// <summary>
	/// エミッタの初期設定をカスタマイズします。
	/// </summary>
	virtual void Initialize(Emitter* emitter) = 0;

	/// <summary>
	/// 個々のパーティクルの更新処理を実装します。
	/// </summary>
	virtual void Update(Particle& particle, float deltaTime) = 0;

	/// <summary>
	/// 新規パーティクル生成時のプロパティをカスタマイズします。
	/// </summary>
	virtual void MakeNewParticle(Particle& particle, std::mt19937& randomEngine, const Emitter& emitter) = 0;

	/// <summary>
	/// この振る舞い固有のデバッグUIを表示します。
	/// </summary>
	virtual void Debug(Emitter* emitter, DebugUI* ui) = 0;
};

/// <summary>
/// 通常のパーティクル（追加の振る舞いなし）
/// </summary>
class NormalBehavior : public IParticleBehavior {
public:
	void Initialize(Emitter* emitter) override;
	void Update(Particle& particle, float deltaTime) override;
	void MakeNewParticle(Particle& particle, std::mt19937& randomEngine, const Emitter& emitter) override;
	void Debug(Emitter* emitter, DebugUI* ui) override;
};

/// <summary>
/// 加速フィールドの振る舞い
/// </summary>
class AccelerationFieldBehavior : public IParticleBehavior {
public:
	void Initialize(Emitter* emitter) override;
	void Update(Particle& particle, float deltaTime) override;
	void MakeNewParticle(Particle& particle, std::mt19937& randomEngine, const Emitter& emitter) override;
	void Debug(Emitter* emitter, DebugUI* ui) override;
private:
	AccelerationField field_;
};

/// <summary>
/// ヒットエフェクトの振る舞い
/// </summary>
class HitEffectBehavior : public IParticleBehavior {
public:
	void Initialize(Emitter* emitter) override;
	void Update(Particle& particle, float deltaTime) override;
	void MakeNewParticle(Particle& particle, std::mt19937& randomEngine, const Emitter& emitter) override;
	void Debug(Emitter* emitter, DebugUI* ui) override;
};

// ファクトリ関数
std::unique_ptr<IParticleBehavior> CreateParticleBehavior(ParticleType type);