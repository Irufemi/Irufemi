#include "OffScreenIndicator.h"
#include "2D/Sprite.h"
#include "camera/Camera.h"
#include "actors/enemy/Enemy.h"
#include "engine/IrufemiEngine.h"
#include "function/Math.h"
#include <numbers>

OffScreenIndicator::OffScreenIndicator() {}

OffScreenIndicator::~OffScreenIndicator() {}

void OffScreenIndicator::Initialize(Camera* camera, IrufemiEngine* engine) {
	camera_ = camera;
	engine_ = engine;

	indicators_.resize(kMaxIndicators_);
	for (auto& indicator : indicators_) {
		indicator.sprite = std::make_unique<Sprite>();
		// TODO: 適切なテクスチャパスに変更してください
		indicator.sprite->Initialize(camera_, "resources/texture/game/!.png"); 
		indicator.sprite->SetAnchor(0.5f, 0.5f);
		indicator.targetEnemy = nullptr;
	}
}

void OffScreenIndicator::Update(const std::list<Enemy*>& attackingEnemies) {
	if (!camera_ || !engine_) return;

	// インジケーターをリセット
	for (auto& indicator : indicators_) {
		indicator.targetEnemy = nullptr;
	}

	int indicatorIndex = 0;
	const float screenWidth = static_cast<float>(engine_->GetClientWidth());
	const float screenHeight = static_cast<float>(engine_->GetClientHeight());
	const float margin = 50.0f; // 画面端からのマージン

	for (Enemy* enemy : attackingEnemies) {
		if (!enemy || !enemy->IsAlive()) continue;
		if (indicatorIndex >= kMaxIndicators_) break;

		Vector3 enemyPos = enemy->GetOBB().center;
		Matrix4x4 viewProjection = Math::Multiply(camera_->GetViewMatrix(), camera_->GetPerspectiveFovMatrix());
		Vector3 screenPos = Math::Transform(enemyPos, viewProjection);

		// ビューポート外か判定
		if (screenPos.x < -1.0f || screenPos.x > 1.0f || screenPos.y < -1.0f || screenPos.y > 1.0f) {
			
			// 画面中央から敵への方向ベクトルを計算
			Vector3 playerPos = camera_->GetTranslate();
			Vector3 direction = Math::Normalize(enemyPos - playerPos);

			// 画面端にクランプ
			float angle = atan2(direction.y, direction.x);
			float cosA = cos(angle);
			float sinA = sin(angle);

			float halfW = (screenWidth / 2.0f) - margin;
			float halfH = (screenHeight / 2.0f) - margin;
			
			float x = 0.0f, y = 0.0f;

			if (halfW * std::abs(sinA) < halfH * std::abs(cosA)) {
				// 上下辺に衝突
				x = (cosA / std::abs(cosA)) * halfW;
				y = (cosA / std::abs(cosA)) * halfW * tan(angle);
			}
			else {
				// 左右辺に衝突
				x = (sinA / std::abs(sinA)) * halfH / tan(angle);
				y = (sinA / std::abs(sinA)) * halfH;
			}

			// スクリーン座標に変換
			float finalX = (screenWidth / 2.0f) + x;
			float finalY = (screenHeight / 2.0f) - y; // Y軸は反転

			// インジケーターを設定
			auto& indicator = indicators_[indicatorIndex];
			indicator.targetEnemy = enemy;
			indicator.sprite->SetPosition(finalX, finalY);
			indicator.sprite->Update();

			indicatorIndex++;
		}
	}
}

void OffScreenIndicator::Draw() {
	for (const auto& indicator : indicators_) {
		if (indicator.targetEnemy != nullptr) {
			indicator.sprite->Draw();
		}
	}
}