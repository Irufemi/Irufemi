#include "Particle.h"
#include "function/Ease.h"
#include "function/Collision.h"

void Particle::Update(float deltaTime) {
	currentTime += deltaTime;
	transform.translate += velocity * deltaTime;

	// 生存期間の進行度を計算 (0.0f -> 1.0f)
	float progress = currentTime / lifeTime;

	// 進行度に基づいてスケールと色を線形補間
	transform.scale = Lerp(startScale, endScale, progress);
	color = Lerp(startColor, endColor, progress);
}

void AccelerationField::Apply(Particle& particle, float deltaTime) const {
	if (Collision::IsCollision(area, particle.transform.translate)) {
		particle.velocity += acceleration * deltaTime;
	}
}