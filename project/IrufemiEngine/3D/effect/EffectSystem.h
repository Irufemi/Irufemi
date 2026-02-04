#pragma once

#include "3D/particle/ParticleSystem.h"
#include "math/Transform.h"
#include <memory>
#include <string>
#include <vector>

// 前方宣言
class Camera;

enum class EffectType {
	kNone, // 無し
	kAura, // シリンダーでのオーラ
	kHitEffect, // 単体RingとParticleSystemのHitEffectを組み合わせた形
	kArmorBreak,
};

/// <summary>
/// エフェクトの振る舞いを定義するインターフェース
/// </summary>
class IEffectBehavior {
public:
	virtual ~IEffectBehavior() = default;
	virtual void Initialize(Camera* camera) = 0;
	virtual void Update() = 0;
	virtual void Draw() = 0;
	virtual void Debug(const std::string& name) = 0;
	virtual void Play(const Transform& transform) = 0;
	virtual bool IsPlaying() const = 0;
};

/// <summary>
/// オーラエフェクト
/// </summary>
class AuraEffect : public IEffectBehavior {
public:
	void Initialize(Camera* camera) override;
	void Update() override;
	void Draw() override;
	void Debug(const std::string& name) override;
	void Play(const Transform& transform) override;
	bool IsPlaying() const override { return isPlaying_; }

private:
	std::unique_ptr<ParticleSystem> particleSystem_;
	Transform transform_{};
	bool isPlaying_ = false;
};

/// <summary>
/// ヒットエフェクト
/// </summary>
class HitEffect : public IEffectBehavior {
public:
	void Initialize(Camera* camera) override;
	void Update() override;
	void Draw() override;
	void Debug(const std::string& name) override;
	void Play(const Transform& transform) override;
	bool IsPlaying() const override { return isPlaying_; }

private:
	std::unique_ptr<ParticleSystem> ringParticle_;
	std::unique_ptr<ParticleSystem> sparkParticle_;
	Transform transform_{};
	bool isPlaying_ = false;
	float lifeTime_ = 0.5f; // エフェクト全体の生存時間
	float currentTime_ = 0.0f;
};

// ファクトリ関数
std::unique_ptr<IEffectBehavior> CreateEffectBehavior(EffectType type);

class ArmorBreakEffect : public IEffectBehavior {
public:
	void Initialize(Camera* camera) override;
	void Update() override;
	void Draw() override;
	void Debug(const std::string& name) override;
	void Play(const Transform& transform) override;
	bool IsPlaying() const override { return isPlaying_; }

private:
	Camera* camera_ = nullptr;
	Transform transform_;
	bool isPlaying_ = false;
	float currentTime_ = 0.0f;
	float lifeTime_ = 1.5f;

	std::unique_ptr<ParticleSystem> debrisParticle_;
	std::unique_ptr<ParticleSystem> shockwaveParticle_;
};

class EffectSystem {
public: // メンバ関数
	// コンストラクタ
	EffectSystem();
	// デストラクタ
	~EffectSystem();
	// 初期化
	void Initialize(Camera* camera);
	// 更新
	void Update();
	// デバッグ
	void Debug(const std::string& name = "");
	// 描画
	void Draw();

	/// <summary>
	/// エフェクトを再生
	/// </summary>
	void Play(EffectType type, const Transform& transform);

private:
	Camera* camera_ = nullptr;
	std::vector<std::unique_ptr<IEffectBehavior>> effects_;

	// UI で使う一時 Transform(位置/回転(rad)/スケール)
	Transform spawnTransform_{ {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {0.0f,0.0f,0.0f} };
	EffectType selectedEffectType_ = EffectType::kAura; // UIで選択されたエフェクトタイプ
};

