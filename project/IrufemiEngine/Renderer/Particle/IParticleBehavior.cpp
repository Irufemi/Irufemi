#include "Renderer/Particle/IParticleBehavior.h"
#include "Engine/Manager/DebugUI.h"
#include "Renderer/Particle/ParticleSystem.h"
#include <numbers>

// NormalBehavior
void NormalBehavior::Initialize(Emitter* emitter) {
	// デフォルト値にリセット
	emitter->count = 3;
	emitter->area = { 2.0f, 2.0f, 2.0f };
	emitter->velocityMin = { -1.0f, -1.0f, -1.0f };
	emitter->velocityMax = { 1.0f, 1.0f, 1.0f };
}
void NormalBehavior::Update([[maybe_unused]] Particle& particle, [[maybe_unused]] float deltaTime) {
	// 何もしない
}
void NormalBehavior::MakeNewParticle(Particle& particle, std::mt19937& randomEngine, const Emitter& emitter) {
	std::uniform_real_distribution<float> distTime(1.0f, 3.0f);
	particle.startScale = emitter.startScale;
	particle.endScale = emitter.endScale;
	particle.transform.rotate = { 0.0f,0.0f,0.0f };
	particle.lifeTime = distTime(randomEngine);
}
void NormalBehavior::Debug([[maybe_unused]] Emitter* emitter, [[maybe_unused]] DebugUI* ui, [[maybe_unused]] ParticleSystem* particleSystem) {
	// Normal固有のUIはなし
}


// AccelerationFieldBehavior
void AccelerationFieldBehavior::Initialize(Emitter* emitter) {
	emitter->count = 3;
	emitter->area = { 2.0f, 2.0f, 2.0f };
	emitter->velocityMin = { -1.0f, -1.0f, -1.0f };
	emitter->velocityMax = { 1.0f, 1.0f, 1.0f };
	field_.acceleration = { 15.0f, 0.0f, 0.0f };
	field_.area.min = { -1.0f, -1.0f, -1.0f };
	field_.area.max = { 1.0f, 1.0f, 1.0f };
}
void AccelerationFieldBehavior::Update(Particle& particle, float deltaTime) {
	field_.Apply(particle, deltaTime);
}
void AccelerationFieldBehavior::MakeNewParticle(Particle& particle, std::mt19937& randomEngine, const Emitter& emitter) {
	std::uniform_real_distribution<float> distTime(1.0f, 3.0f);
	particle.startScale = emitter.startScale;
	particle.endScale = emitter.endScale;
	particle.transform.rotate = { 0.0f,0.0f,0.0f };
	particle.lifeTime = distTime(randomEngine);
}
void AccelerationFieldBehavior::Debug([[maybe_unused]] Emitter* emitter, DebugUI* ui, ParticleSystem* particleSystem) {
#ifdef USE_IMGUI

	ImGui::DragFloat3("Acceleration", &field_.acceleration.x, 0.1f);
	ImGui::DragFloat3("Area Min", &field_.area.min.x, 0.1f);
	ImGui::DragFloat3("Area Max", &field_.area.max.x, 0.1f);

	// ParticleSystem のフラグを確認してから描画
	if (particleSystem && particleSystem->IsShowFieldAABB()) {
		particleSystem->DrawAABB(field_.area, { 1.0f, 0.0f, 0.0f, 1.0f });
	}

#endif // USE_IMGUI
}


// HitEffectBehavior
void HitEffectBehavior::Initialize(Emitter* emitter) {
	emitter->count = 8;
	emitter->area = { 0.0f, 0.0f, 0.0f };
	emitter->velocityMin = { 0.0f, 0.0f, 0.0f };
	emitter->velocityMax = { 0.0f, 0.0f, 0.0f };
}
void HitEffectBehavior::Update([[maybe_unused]] Particle& particle, [[maybe_unused]] float deltaTime) {
	// 何もしない
}
void HitEffectBehavior::MakeNewParticle(Particle& particle, std::mt19937& randomEngine, const Emitter& emitter) {
	std::uniform_real_distribution<float> distRotate(-std::numbers::pi_v<float>, std::numbers::pi_v<float>);
	std::uniform_real_distribution<float> distScale(0.4f, 1.5f);
	particle.startScale = { 0.1f, distScale(randomEngine), 1.0f };
	particle.endScale = particle.startScale * 0.3f;
	particle.transform.rotate = { 0.0f, 0.0f, distRotate(randomEngine) };
	particle.lifeTime = 0.5f; // ヒットエフェクトは短命に
}
void HitEffectBehavior::Debug([[maybe_unused]] Emitter* emitter, [[maybe_unused]] DebugUI* ui, [[maybe_unused]] ParticleSystem* particleSystem) {
	// HitEffect固有のUIはなし
}

// ExplosionBehavior
void ExplosionBehavior::Initialize(Emitter* emitter) {
	emitter->count = 20;
	emitter->area = { 0.0f, 0.0f, 0.0f };
	emitter->velocityMin = { -5.0f, -5.0f, -5.0f };
	emitter->velocityMax = { 5.0f, 5.0f, 5.0f };
	field_.acceleration = { 0.0f, -9.8f, 0.0f }; // 重力
	field_.area.min = { -100.0f, -100.0f, -100.0f }; // 広範囲に適用
	field_.area.max = { 100.0f, 100.0f, 100.0f };
}
void ExplosionBehavior::Update(Particle& particle, float deltaTime) {
	field_.Apply(particle, deltaTime);
}
void ExplosionBehavior::MakeNewParticle(Particle& particle, std::mt19937& randomEngine, const Emitter& emitter) {
	std::uniform_real_distribution<float> distRotate(-std::numbers::pi_v<float>, std::numbers::pi_v<float>);
	std::uniform_real_distribution<float> distScale(0.4f, 1.0f);
	std::uniform_real_distribution<float> distTime(1.5f, 3.0f);

	particle.startScale = { distScale(randomEngine), distScale(randomEngine), 1.0f };
	particle.endScale = particle.startScale * 0.1f;
	particle.transform.rotate = { 0.0f, 0.0f, distRotate(randomEngine) };
	particle.lifeTime = distTime(randomEngine);
}
void ExplosionBehavior::Debug([[maybe_unused]] Emitter* emitter, DebugUI* ui, ParticleSystem* particleSystem) {
#ifdef USE_IMGUI
    ImGui::DragFloat3("Acceleration", &field_.acceleration.x, 0.1f);
    ImGui::DragFloat3("Area Min", &field_.area.min.x, 0.1f);
    ImGui::DragFloat3("Area Max", &field_.area.max.x, 0.1f);

    if (particleSystem && particleSystem->IsShowFieldAABB()) {
        particleSystem->DrawAABB(field_.area, { 1.0f, 0.0f, 0.0f, 1.0f });
    }
#endif // USE_IMGUI
}


// MuzzleSmokeBehavior
void MuzzleSmokeBehavior::Initialize(Emitter* emitter) {
	emitter->count = 3;
	emitter->area = { 0.1f, 0.1f, 0.1f };
	emitter->velocityMin = { -0.5f, 0.2f, -0.5f };
	emitter->velocityMax = { 0.5f, 1.0f, 0.5f };
	emitter->startScale = { 0.1f, 0.1f, 1.0f };
	emitter->endScale = { 0.5f, 0.5f, 1.0f };
	emitter->startColor = { 1.0f, 1.0f, 1.0f, 0.5f };
	emitter->endColor = { 1.0f, 1.0f, 1.0f, 0.0f };
	emitter->colorMode = ParticleColorMode::kNone;
}
void MuzzleSmokeBehavior::Update(Particle& particle, [[maybe_unused]] float deltaTime) {
	// 速度の減衰(空気抵抗)
	particle.velocity *= 0.95f;
	// 浮力(少し浮上)
	particle.velocity.y += 0.5f * deltaTime;
}
void MuzzleSmokeBehavior::MakeNewParticle(Particle& particle, std::mt19937& randomEngine, const Emitter& emitter) {
	std::uniform_real_distribution<float> distTime(0.5f, 1.2f);
	std::uniform_real_distribution<float> distScale(0.8f, 1.2f);
	std::uniform_real_distribution<float> distRotate(-std::numbers::pi_v<float>, std::numbers::pi_v<float>);

	particle.startScale = emitter.startScale * distScale(randomEngine);
	particle.endScale = emitter.endScale * distScale(randomEngine);
	particle.transform.rotate = { 0.0f, 0.0f, distRotate(randomEngine) };
	particle.lifeTime = distTime(randomEngine);
}
void MuzzleSmokeBehavior::Debug([[maybe_unused]] Emitter* emitter, [[maybe_unused]] DebugUI* ui, [[maybe_unused]] ParticleSystem* particleSystem) {
	// 必要に応じてUIを追加
}

// MuzzleFlashBehavior
void MuzzleFlashBehavior::Initialize(Emitter* emitter) {
	emitter->count = 50; // さらに倍増！
	emitter->area = { 0.01f, 0.01f, 0.01f }; // 放出点を中心に固める
	emitter->velocityMin = { -15.0f, -15.0f, -15.0f }; // 爆発的な速度
	emitter->velocityMax = { 15.0f, 15.0f, 15.0f };
	emitter->startScale = { 1.0f, 1.0f, 1.0f }; // ダミー（MakeNewParticleで設定）
	emitter->endScale = { 0.0f, 0.0f, 1.0f };
	emitter->startColor = { 1.0f, 0.7f, 0.1f, 1.0f }; // 鮮やかなオレンジ（通常ブレンド用に正規化）
	emitter->endColor = { 1.0f, 0.0f, 0.0f, 0.0f }; // 赤く消えていく
	emitter->colorMode = ParticleColorMode::kNone;
}
void MuzzleFlashBehavior::Update(Particle& particle, float deltaTime) {
	// 一瞬で広がり、一瞬で消えるための急激な減衰
	particle.velocity *= 0.3f; 
}
void MuzzleFlashBehavior::MakeNewParticle(Particle& particle, std::mt19937& randomEngine, const Emitter& emitter) {
	std::uniform_real_distribution<float> distType(0.0f, 1.0f);
	std::uniform_real_distribution<float> distTime(0.06f, 0.13f); // 残像感のある寿命
	std::uniform_real_distribution<float> distRotate(-std::numbers::pi_v<float>, std::numbers::pi_v<float>);

	float typeRoll = distType(randomEngine);
	if (typeRoll < 0.3f) {
		// 【コア】 中心で大きく光る塊
		std::uniform_real_distribution<float> distScale(0.5f, 1.0f); // 極小に
		float s = distScale(randomEngine);
		particle.startScale = { s, s, 1.0f };
	} else {
		// 【ライン】 放射状に伸びる鋭い閃光
		std::uniform_real_distribution<float> distWidth(0.05f, 0.1f); // 極細
		std::uniform_real_distribution<float> distLength(0.5f, 1.5f); // 非常に短く
		particle.startScale = { distWidth(randomEngine), distLength(randomEngine), 1.0f };
		// 速度をラインの方向に合わせる（より放射状に見せる）
		float angle = distRotate(randomEngine);
		particle.transform.rotate.z = angle;
		float speed = 20.0f;
		particle.velocity = { std::sin(angle) * speed, -std::cos(angle) * speed, 0.0f };
	}
	
	particle.transform.rotate.z = distRotate(randomEngine);
	particle.endScale = emitter.endScale;
	particle.lifeTime = distTime(randomEngine);
}
void MuzzleFlashBehavior::Debug([[maybe_unused]] Emitter* emitter, [[maybe_unused]] DebugUI* ui, [[maybe_unused]] ParticleSystem* particleSystem) {
#ifdef USE_IMGUI
	ImGui::Text("Muzzle Flash Particle Behavior");
#endif
}

// MissileFireBehavior
void MissileFireBehavior::Initialize(Emitter* emitter) {
	emitter->count = 5;
	emitter->area = { 0.1f, 0.1f, 0.1f };
	emitter->velocityMin = { -1.0f, -1.0f, -1.0f };
	emitter->velocityMax = { 1.0f, 1.0f, 1.0f };
	emitter->startScale = { 0.5f, 0.5f, 0.5f };
	emitter->endScale = { 0.1f, 0.1f, 0.1f };
	emitter->startColor = { 2.0f, 1.5f, 0.5f, 1.0f }; // 高輝度オレンジ
	emitter->endColor = { 1.0f, 0.0f, 0.0f, 0.0f };
	emitter->colorMode = ParticleColorMode::kNone;
}
void MissileFireBehavior::Update(Particle& particle, float deltaTime) {
	// 推進炎なので少し揺らぐ
	particle.velocity.x += (std::rand() % 100 / 100.0f - 0.5f) * 0.5f;
	particle.velocity.y += (std::rand() % 100 / 100.0f - 0.5f) * 0.5f;
	particle.velocity.z += (std::rand() % 100 / 100.0f - 0.5f) * 0.5f;
}
void MissileFireBehavior::MakeNewParticle(Particle& particle, std::mt19937& randomEngine, const Emitter& emitter) {
	std::uniform_real_distribution<float> distTime(0.05f, 0.15f);
	std::uniform_real_distribution<float> distScale(0.8f, 1.5f);
	particle.startScale = emitter.startScale * distScale(randomEngine);
	particle.endScale = emitter.endScale;
	particle.lifeTime = distTime(randomEngine);
}
void MissileFireBehavior::Debug([[maybe_unused]] Emitter* emitter, [[maybe_unused]] DebugUI* ui, [[maybe_unused]] ParticleSystem* particleSystem) {}

// MissileSmokeBehavior
void MissileSmokeBehavior::Initialize(Emitter* emitter) {
	emitter->count = 3;
	emitter->area = { 0.2f, 0.2f, 0.2f };
	emitter->velocityMin = { -0.5f, -0.5f, -0.5f };
	emitter->velocityMax = { 0.5f, 0.5f, 0.5f };
	emitter->startScale = { 0.3f, 0.3f, 0.3f };
	emitter->endScale = { 1.5f, 1.5f, 1.5f }; // 広がる煙
	emitter->startColor = { 0.5f, 0.5f, 0.5f, 0.6f };
	emitter->endColor = { 0.2f, 0.2f, 0.2f, 0.0f };
	emitter->colorMode = ParticleColorMode::kNone;
}
void MissileSmokeBehavior::Update(Particle& particle, float deltaTime) {
	particle.velocity *= 0.98f; // 空気抵抗で減速
}
void MissileSmokeBehavior::MakeNewParticle(Particle& particle, std::mt19937& randomEngine, const Emitter& emitter) {
	std::uniform_real_distribution<float> distTime(0.5f, 1.2f); // 長生き
	std::uniform_real_distribution<float> distScale(0.5f, 2.0f);
	particle.startScale = emitter.startScale * distScale(randomEngine);
	particle.endScale = emitter.endScale * distScale(randomEngine);
	particle.lifeTime = distTime(randomEngine);
}
void MissileSmokeBehavior::Debug([[maybe_unused]] Emitter* emitter, [[maybe_unused]] DebugUI* ui, [[maybe_unused]] ParticleSystem* particleSystem) {}

// ファクトリ関数
std::unique_ptr<IParticleBehavior> CreateParticleBehavior(ParticleType type) {
	switch (type) {
	case ParticleType::kAccelerationField:
		return std::make_unique<AccelerationFieldBehavior>();
	case ParticleType::kHitEffect:
		return std::make_unique<HitEffectBehavior>();
	case ParticleType::kExplosion:
		return std::make_unique<ExplosionBehavior>();
	case ParticleType::kMuzzleSmoke:
		return std::make_unique<MuzzleSmokeBehavior>();
	case ParticleType::kMuzzleFlash:
		return std::make_unique<MuzzleFlashBehavior>();
	case ParticleType::kMissileFire:
		return std::make_unique<MissileFireBehavior>();
	case ParticleType::kMissileSmoke:
		return std::make_unique<MissileSmokeBehavior>();
	case ParticleType::Normal:
	default:
		return std::make_unique<NormalBehavior>();
	}
}