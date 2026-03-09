#include "IParticleBehavior.h"
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


// ファクトリ関数
std::unique_ptr<IParticleBehavior> CreateParticleBehavior(ParticleType type) {
	switch (type) {
	case ParticleType::kAccelerationField:
		return std::make_unique<AccelerationFieldBehavior>();
	case ParticleType::kHitEffect:
		return std::make_unique<HitEffectBehavior>();
	case ParticleType::kExplosion:
		return std::make_unique<ExplosionBehavior>();
	case ParticleType::Normal:
	default:
		return std::make_unique<NormalBehavior>();
	}
}