#include "EffectSystem.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Manager/DebugUI.h"

// --- AuraEffect ---
void AuraEffect::Initialize(Camera* camera) {
	particleSystem_ = std::make_unique<ParticleSystem>();
	// オーラ用のパーティクル設定
	particleSystem_->Initialize(camera, "resources/gradationLine.png", ParticleType::Normal, PrimitiveType::Cylinder);
	particleSystem_->SetEmitterArea({ 0.0f, 0.0f, 0.0f });
	particleSystem_->SetEmitterCount(1);
	particleSystem_->SetEmitterFrequency(100.0f); // 自動では発生させない
	particleSystem_->SetParticleScale({ 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f });
	particleSystem_->SetParticleColor({ 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 0.0f });
	particleSystem_->SetCylinderParameters(0.8f, 2.0f, 32, true);
}

void AuraEffect::Update() {
	if (isPlaying_) {
		// Transformの追従など
		particleSystem_->SetEmitterPosition(transform_.translate);
		particleSystem_->Update();
	}
}

void AuraEffect::Draw() {
	if (isPlaying_) {
		particleSystem_->Draw();
	}
}

void AuraEffect::Debug(const std::string& name) {
#ifdef USE_IMGUI
	if (ImGui::TreeNode(name.c_str())) {
		ImGui::Checkbox("IsPlaying", &isPlaying_);
		DebugUI::TextTransform(transform_, "Aura Transform");
		particleSystem_->Debug("Aura Particle");
		ImGui::TreePop();
	}
#endif
}

void AuraEffect::Play(const Transform& transform) {
	transform_ = transform;
	isPlaying_ = true;
	// ParticleSystemは追従するだけなので、Play時に何か特別なことをする必要は少ない
}

// --- HitEffect ---
void HitEffect::Initialize(Camera* camera) {
	// リング状エフェクト
	ringParticle_ = std::make_unique<ParticleSystem>();
	ringParticle_->Initialize(camera, "resources/gradationLine.png", ParticleType::Normal, PrimitiveType::Ring);
	ringParticle_->SetEmitterCount(1);
	ringParticle_->SetEmitterFrequency(100.0f);
	ringParticle_->SetParticleScale({ 0.1f, 0.1f, 1.0f }, { 2.0f, 2.0f, 1.0f });
	ringParticle_->SetParticleColor({ 1.0f, 1.0f, 0.5f, 1.0f }, { 1.0f, 0.5f, 0.0f, 0.0f });
	ringParticle_->SetRingParameters(0.2f, 0.5f, 0.0f, 360.0f, 32);

	// 火花エフェクト
	sparkParticle_ = std::make_unique<ParticleSystem>();
	sparkParticle_->Initialize(camera, "resources/circle.png", ParticleType::kHitEffect, PrimitiveType::Plane);
}

void HitEffect::Update() {
	if (isPlaying_) {
		currentTime_ += 1.0f / 60.0f;
		if (currentTime_ >= lifeTime_) {
			isPlaying_ = false;
		}
		ringParticle_->Update();
		sparkParticle_->Update();
	}
}

void HitEffect::Draw() {
	if (isPlaying_) {
		ringParticle_->Draw();
		sparkParticle_->Draw();
	}
}

void HitEffect::Debug(const std::string& name) {
#ifdef USE_IMGUI
	if (ImGui::TreeNode(name.c_str())) {
		ImGui::Checkbox("IsPlaying", &isPlaying_);
		DebugUI::TextTransform(transform_, "Hit Transform");
		ringParticle_->Debug("Ring Particle");
		sparkParticle_->Debug("Spark Particle");
		ImGui::TreePop();
	}
#endif
}

void HitEffect::Play(const Transform& transform) {
	transform_ = transform;
	isPlaying_ = true;
	currentTime_ = 0.0f;

	// リングと火花を再生
	ringParticle_->SetEmitterPosition(transform.translate);
	// 向きを反映させるためにビルボードを切る
	// ※現状ParticleSystemに回転を直接渡す機能がないため、これは概念的なコードです
	//   もし回転させたい場合はParticleSystemの拡張が必要です。
	ringParticle_->PlayHitEffect(transform.translate);
	sparkParticle_->PlayHitEffect(transform.translate);
}


// --- ファクトリ関数 ---
std::unique_ptr<IEffectBehavior> CreateEffectBehavior(EffectType type) {
	switch (type) {
	case EffectType::kAura:
		return std::make_unique<AuraEffect>();
	case EffectType::kHitEffect:
		return std::make_unique<HitEffect>();
	default:
		return nullptr;
	}
}


// --- EffectSystem ---
EffectSystem::EffectSystem() {}

EffectSystem::~EffectSystem() {}

void EffectSystem::Initialize(Camera* camera) {
	camera_ = camera;
	effects_.clear();
}

void EffectSystem::Update() {
	for (auto& effect : effects_) {
		effect->Update();
	}
	// 再生が終わったエフェクトを削除
	effects_.erase(
		std::remove_if(
			effects_.begin(), effects_.end(), [](const auto& effect) { return !effect->IsPlaying(); }),
		effects_.end());
}

void EffectSystem::Draw() {
	for (auto& effect : effects_) {
		effect->Draw();
	}
}

void EffectSystem::Debug(const std::string& name) {
#ifdef USE_IMGUI
	ImGui::Begin(name.c_str());

	// エフェクト再生UI
	if (ImGui::CollapsingHeader("Spawn Controls")) {
		// エフェクトタイプの選択
		const char* effectTypeNames[] = { "None", "Aura", "HitEffect" };
		int currentType = static_cast<int>(selectedEffectType_);
		if (ImGui::Combo("Effect Type", &currentType, effectTypeNames, IM_ARRAYSIZE(effectTypeNames))) {
			selectedEffectType_ = static_cast<EffectType>(currentType);
		}

		// Transformの編集
		DebugUI::TextTransform(spawnTransform_, "Spawn Transform");

		// 再生ボタン
		if (ImGui::Button("Play Effect")) {
			if (selectedEffectType_ != EffectType::kNone) {
				Play(selectedEffectType_, spawnTransform_);
			}
		}
	}

	ImGui::Separator();

	// 再生中のエフェクトリスト
	ImGui::Text("Active Effects: %zu", effects_.size());
	int index = 0;
	for (auto& effect : effects_) {
		std::string effectName = "Effect " + std::to_string(index++);
		effect->Debug(effectName);
	}
	ImGui::End();
#endif
}

void EffectSystem::Play(EffectType type, const Transform& transform) {
	// 新しいエフェクトインスタンスを作成
	auto newEffect = CreateEffectBehavior(type);
	if (newEffect) {
		newEffect->Initialize(camera_);
		newEffect->Play(transform);
		effects_.push_back(std::move(newEffect));
	}
}