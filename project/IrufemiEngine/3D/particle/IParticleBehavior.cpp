#include "IParticleBehavior.h"
#include "manager/DebugUI.h"
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
void NormalBehavior::Debug([[maybe_unused]] Emitter* emitter, [[maybe_unused]] DebugUI* ui) {
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
void AccelerationFieldBehavior::Debug([[maybe_unused]] Emitter* emitter, DebugUI* ui) {
#ifdef USE_IMGUI

	ImGui::DragFloat3("Acceleration", &field_.acceleration.x, 0.1f);
	ImGui::DragFloat3("Area Min", &field_.area.min.x, 0.1f);
	ImGui::DragFloat3("Area Max", &field_.area.max.x, 0.1f);

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
void HitEffectBehavior::Debug([[maybe_unused]] Emitter* emitter, [[maybe_unused]] DebugUI* ui) {
	// HitEffect固有のUIはなし
}


// ファクトリ関数
std::unique_ptr<IParticleBehavior> CreateParticleBehavior(ParticleType type) {
	switch (type) {
	case ParticleType::kAccelerationField:
		return std::make_unique<AccelerationFieldBehavior>();
	case ParticleType::kHitEffect:
		return std::make_unique<HitEffectBehavior>();
	case ParticleType::Normal:
	default:
		return std::make_unique<NormalBehavior>();
	}
}