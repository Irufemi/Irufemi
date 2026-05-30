#include "Phase2_Tackle.h"
#include "Enemy.h"
#include "Engine/Core/Math/Math.h"
#include "actors/enemy/EnemyParameters.h"
#include "actors/player/Player.h"
#include <algorithm>
#include <cmath>

    void Phase2_Tackle::Enter(Enemy *enemy) {
  timer_ = 0.0f;
  isFinished_ = false;
  isTargetLocked_ = false;
  hasPlayedRushSe_ = false;
  attackTarget_ = {0, 0, 0};
  rushDir_ = {0, 0, 0};
}

void Phase2_Tackle::Update(Enemy *enemy, Player *player, float deltaTime) {
  if (!enemy)
    return;

  timer_ += deltaTime;
  Vector3 playerPos = (player) ? player->GetTranslate() : Vector3{0, 0, 0};

  // 操作対象の首のトランスフォームを取得
  Transform *headT = nullptr;
  if (headIndex_ == 0)
    headT = &enemy->GetHeadLeftLocalTransform();
  else if (headIndex_ == 1)
    headT = &enemy->GetHeadMidLocalTransform();
  else
    headT = &enemy->GetHeadRightLocalTransform();

  if (!headT)
    return;

  if (timer_ < kOrbitTime) {
    // --- 旋回中は常にプレイヤーを向く ---
    Vector3 toPlayer = Math::Subtract(playerPos, headT->translate);
    headT->rotate.y = std::atan2(toPlayer.x, toPlayer.z);
    float distXZ = std::sqrt(toPlayer.x * toPlayer.x + toPlayer.z * toPlayer.z);
    headT->rotate.x = -std::atan2(toPlayer.y, distXZ);

    // 旋回しながら近づく
    float angleOffset = (float)headIndex_ * (Math::PI * 2.0f / 3.0f);
    float currentAngle = timer_ * kOrbitSpeed + angleOffset;

    Vector3 targetPos = playerPos;
    targetPos.x += std::cos(currentAngle) * kOrbitRadius;
    targetPos.z += std::sin(currentAngle) * kOrbitRadius;
    targetPos.y += 2.0f;

    headT->translate.x += (targetPos.x - headT->translate.x) * 0.1f;
    headT->translate.y += (targetPos.y - headT->translate.y) * 0.1f;
    headT->translate.z += (targetPos.z - headT->translate.z) * 0.1f;
  } else if (timer_ < kOrbitTime + kStopTime) {
    if (!isTargetLocked_) {
      // この瞬間にターゲット位置を確定させる
      attackTarget_ = playerPos;

      // ターゲットの方にしっかり向きを固定する
      Vector3 toTarget = Math::Subtract(attackTarget_, headT->translate);
      headT->rotate.y = std::atan2(toTarget.x, toTarget.z);
      float distXZ =
          std::sqrt(toTarget.x * toTarget.x + toTarget.z * toTarget.z);
      headT->rotate.x = -std::atan2(toTarget.y, distXZ);

      // 突進方向をここで計算・保持して直線のみにする！
      rushDir_ = Math::Normalize(toTarget);

      if (auto effects = enemy->GetTackleEffects()) {
        float headWidth =
            EnemyParameters::GetInstance()->GetHeadOBBSize().x * 2.0f;
        float waveWidth = effects->GetMaxRushWaveWidth();
        float maxWidth = (std::max)(headWidth, waveWidth);
        effects->StartTelegraph(headT->translate, headT->rotate.y, 300.0f,
                                maxWidth);
      }

      isTargetLocked_ = true;
    }

    // --- 位置の移動を完全に停止する（明確な隙の提示） ---
    // わずかに顔を上げる（口を開ける溜め）動作だけ行い、translateはいじらない。
    // こうすることで「場所を決めた」「いまからここに突っ込む」という予備動作になる。
    Vector3 toTarget = Math::Subtract(attackTarget_, headT->translate);
    float distXZ = std::sqrt(toTarget.x * toTarget.x + toTarget.z * toTarget.z);
    float targetRotX = -std::atan2(toTarget.y, distXZ) - 0.5f; // 上を向く

    headT->rotate.x += (targetRotX - headT->rotate.x) * 0.15f;

    if (auto effects = enemy->GetTackleEffects()) {
      float warningRatio = (timer_ - kOrbitTime) / kStopTime;
      effects->UpdateTelegraph(headT->translate, headT->rotate.y, warningRatio);
    }
  } else if (timer_ < kOrbitTime + kStopTime + kRushTime) {
    if (auto effects = enemy->GetTackleEffects()) {
      effects->StopTelegraph();
    }
    if (!hasPlayedRushSe_ && enemy) {
        enemy->PlaySeRush();
        hasPlayedRushSe_ = true;
    }
    // 突進：溜めた顔を勢いよく下に振り下ろしながら目標方向への完全な直進を行う
    Vector3 toTarget = Math::Subtract(attackTarget_, headT->translate);
    // 顔の向きだけ下に向ける（振り下ろす）
    headT->rotate.x +=
        (-std::atan2(toTarget.y, std::sqrt(toTarget.x * toTarget.x +
                                           toTarget.z * toTarget.z)) +
         0.3f - headT->rotate.x) *
        0.2f;

    // 計算しておいた直線方向 (rushDir_) にだけ進む
    headT->translate =
        Math::Add(headT->translate, Math::Multiply(kRushSpeed, rushDir_));
  } else {
    isFinished_ = true;
  }
}

void Phase2_Tackle::Exit(Enemy *enemy) {
  if (enemy) {
    if (auto effects = enemy->GetTackleEffects()) {
      effects->StopTelegraph();
    }
  }
}
