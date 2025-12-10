#include "Enemy.h"
#include "actor/player/Player.h"
#include "manager/DebugUI.h"
#include <cmath>
#include <random>

namespace {

// 乱数生成器を取得（初期化は一度だけ）
std::mt19937 &GetRngEnemy() {
  static std::random_device rd;
  static std::mt19937 mt(rd());
  return mt;
}

// min〜max の範囲でランダムな実数を返す
float RandomRangeEnemy(float minValue, float maxValue) {
  std::uniform_real_distribution<float> dist(minValue, maxValue);
  return dist(GetRngEnemy());
}

// min〜max の範囲でランダムな整数を返す
int RandomIntEnemy(int minValue, int maxValue) {
  std::uniform_int_distribution<int> dist(minValue, maxValue);
  return dist(GetRngEnemy());
}

// 2点の XZ 平面上の距離を返す
float DistanceXZ(const Vector3 &a, const Vector3 &b) {
  float dx = a.x - b.x;
  float dz = a.z - b.z;
  return std::sqrt(dx * dx + dz * dz);
}

} // namespace

Enemy::Enemy() {}

void Enemy::Initialize(Camera *camera, const Vector3 &spawnPos,
                       const float &stageRadius, EnemyWallManager *wallManager,
                       EnemyBulletManager *bulletManager) {
  camera_ = camera;

  model_ = std::make_unique<ObjClass>();
  model_->Initialize(camera_, "boss.obj");

  transform_.translate = spawnPos;
  transform_.scale = {3.0f, 3.0f, 3.0f};

  enemyWall_ = wallManager;
  enemyBullet_ = bulletManager;

  // HP 初期化（フェーズ1開始）
  phase_ = EnemyPhase::Phase1;
  hp_ = maxHp_;

  // サイズ変更に合わせて、半径ぶん敵を持ち上げる
  enemyHeightOffset_ = enemyBodyRadius_;
  transform_.translate.y += enemyHeightOffset_;

  // ステージ情報はとりあえず固定値にしておく
  stageCenter_ = Vector3{0.0f, 0.0f, 0.0f};
  stageRadius_ = stageRadius;

  // 地面の高さ（上下の基準）: 持ち上げ後の高さを基準にする
  groundY_ = transform_.translate.y;

  isBurrowing_ = false;
  burrowTimer_ = 0.0f;

  lastHitResult_ = EnemyPlayerHitResult{};

  // 行動状態関連の初期化
  state_ = EnemyState::Idle;
  stateTimer_ = 0.0f;
  bulletChargeProgress_ = 0.0f;
  burrowPhase_ = 0.0f;

  // 弾予備動作用の基準値
  bulletChargeBaseScale_ = transform_.scale;
  bulletChargeBaseHeight_ = transform_.translate.y;

  ResetActionTimer();

  if (model_) {
    model_->SetPosition(transform_.translate);
    model_->Update();
  }
}

void Enemy::Update(float deltaTime, const Vector3 &playerPos) {
  // 壁も弾も管理できないなら何もしない
  if (!enemyWall_ && !enemyBullet_) {
    return;
  }

  // 現在の状態に応じて処理を分ける（予備動作込みの状態マシン）
  switch (state_) {
  case EnemyState::Idle: {
    // 次の行動までの待ち時間
    actionTimer_ -= deltaTime;
    if (actionTimer_ <= 0.0f) {

      // その行動をそもそも実行できるかどうか
      bool canWall = (enemyWall_ != nullptr) && enableWallAttack_;
      bool canBullet = (enemyBullet_ != nullptr) && enableBulletAttack_;
      bool canBurrow = enableBurrowMove_;
      bool canDash = enableDashAttack_;

      // もし「場外ダッシュ後に強制ワープ」フラグが立っていれば、
      // 次の行動は確定で潜り移動にする
      if (forceBurrowOnNextAction_ && canBurrow) {
        // 通常の潜り開始と同じ初期化
        state_ = EnemyState::BurrowPreDive;
        stateTimer_ = burrowPreDuration_;
        burrowPhase_ = 0.0f;
        isBurrowing_ = false; // もぞもぞ中は見えている
        groundY_ = transform_.translate.y;

        // フラグはここで使い切る
        forceBurrowOnNextAction_ = false;
        // ランダム抽選には行かず、このまま潜りへ
        break;
      }

      // 潜りが禁止されていた場合は、フラグだけ落として通常抽選に戻す
      if (forceBurrowOnNextAction_ && !canBurrow) {
        forceBurrowOnNextAction_ = false;
      }

      // プレイヤーとの距離で近/中/遠の重みテーブルを選択
      float dist = DistanceXZ(transform_.translate, playerPos);

      ActionWeightSet weights{};
      if (dist < 4.0f) {
        // 近距離
        weights = weightsNear_;
      } else if (dist >= 8.0f) {
        // 遠距離
        weights = weightsFar_;
      } else {
        // 中距離
        weights = weightsMid_;
      }

      // 実行不能な行動は重みを 0 にする
      if (!canWall) {
        weights.wall = 0.0f;
      }
      if (!canBullet) {
        weights.bullet = 0.0f;
      }
      if (!canBurrow) {
        weights.burrow = 0.0f;
      }
      if (!canDash) {
        weights.dash = 0.0f;
      }

      float totalWeight =
          weights.wall + weights.bullet + weights.burrow + weights.dash;

      // 何もできない場合は行動をスキップして、しばらく待つ
      if (totalWeight <= 0.0f) {
        ResetActionTimer();
        break;
      }

      // 重み付きランダムで行動を決定
      float r = RandomRangeEnemy(0.0f, totalWeight);
      int action = -1;

      float accum = 0.0f;

      accum += weights.wall;
      if (r < accum) {
        action = 0; // 壁
      } else {
        accum += weights.bullet;
        if (r < accum) {
          action = 1; // 弾
        } else {
          accum += weights.burrow;
          if (r < accum) {
            action = 2; // 潜り
          } else {
            action = 3; // 突進
          }
        }
      }

      switch (action) {
      case 0: // 壁攻撃：上昇 → 落下 → 壁生成
        if (canWall) {
          state_ = EnemyState::WallRise;
          stateTimer_ = wallRiseDuration_;
          // 上下の基準は現在位置の Y
          groundY_ = transform_.translate.y;
        }
        break;
      case 1: // 弾攻撃：チャージ → 発射
        if (canBullet) {
          state_ = EnemyState::BulletCharge;
          stateTimer_ = bulletChargeDuration_;
          bulletChargeProgress_ = 0.0f;

          // 膨張・収縮の基準スケール＆高さを記録
          bulletChargeBaseScale_ = transform_.scale;
          bulletChargeBaseHeight_ = transform_.translate.y;
        }
        break;
      case 2: // 潜り移動：もぞもぞ → 地中 → もぞもぞ
        if (canBurrow) {
          state_ = EnemyState::BurrowPreDive;
          stateTimer_ = burrowPreDuration_;
          burrowPhase_ = 0.0f;
          isBurrowing_ = false; // もぞもぞ中は見えている
          groundY_ = transform_.translate.y;
        }
        break;
      case 3: { // 突進攻撃
        if (canDash) {
          state_ = EnemyState::ChargeBack;
          stateTimer_ = dashChargeBackTime_;
          dashMoved_ = 0.0f;

          // プレイヤー方向を保存（突進開始時に使う）
          dashDirection_.x = playerPos.x - transform_.translate.x;
          dashDirection_.y = 0.0f;
          dashDirection_.z = playerPos.z - transform_.translate.z;

          float len = std::sqrt(dashDirection_.x * dashDirection_.x +
                                dashDirection_.z * dashDirection_.z);

          if (len > 0.01f) {
            dashDirection_.x /= len;
            dashDirection_.z /= len;
          }
        }
        break;
      }
      default:
        break;
      }
    }
    break;
  }

  case EnemyState::WallRise: {
    // ドッスンのように上昇する
    stateTimer_ -= deltaTime;
    float t = 1.0f - (stateTimer_ / wallRiseDuration_);
    if (t < 0.0f) {
      t = 0.0f;
    }
    if (t > 1.0f) {
      t = 1.0f;
    }
    transform_.translate.y = groundY_ + wallRiseHeight_ * t;

    if (stateTimer_ <= 0.0f) {
      // 上がり切ったら落下フェーズへ
      state_ = EnemyState::WallDrop;
      stateTimer_ = wallDropDuration_;
    }
    break;
  }

  case EnemyState::WallDrop: {
    // 落下して地面に着地したタイミングで壁生成
    stateTimer_ -= deltaTime;
    float t = 1.0f - (stateTimer_ / wallDropDuration_);
    if (t < 0.0f) {
      t = 0.0f;
    }
    if (t > 1.0f) {
      t = 1.0f;
    }
    // 上に上がっていたぶんを戻す感じ
    transform_.translate.y = groundY_ + wallRiseHeight_ * (1.0f - t);

    if (stateTimer_ <= 0.0f) {
      // 地面に着地したので壁生成
      transform_.translate.y = groundY_;
      if (enemyWall_) {
        // フェーズで壁パターンを切り替える
        if (phase_ == EnemyPhase::Phase2) {
          // フェーズ2：3×1 のライン状の壁を生成
          enemyWall_->SpawnWallLine3x1(transform_.translate, playerPos);
        } else {
          // フェーズ1：従来どおりランダム配置
          enemyWall_->SpawnWalls(transform_.translate, playerPos);
        }
      }
      // 次の行動待ちへ
      state_ = EnemyState::Idle;
      ResetActionTimer();
    }
    break;
  }

  case EnemyState::BulletCharge: {
    // 弾チャージ中（膨らみながらチャージ）
    stateTimer_ -= deltaTime;

    if (stateTimer_ > 0.0f) {
      // 0.0〜1.0 のチャージ進行度
      float t = 1.0f - (stateTimer_ / bulletChargeDuration_);
      if (t < 0.0f) {
        t = 0.0f;
      }
      if (t > 1.0f) {
        t = 1.0f;
      }
      bulletChargeProgress_ = t;

      // 見た目スケールを少しずつ大きくする
      float baseScaleY = bulletChargeBaseScale_.y;
      if (baseScaleY <= 0.0f) {
        baseScaleY = 1.0f;
      }

      float scaleFactor = 1.0f + bulletChargeScaleAmount_ * t; // 最大で 1+0.25
      float newScale = baseScaleY * scaleFactor;

      // 等方スケールで膨らませる
      transform_.scale = {newScale, newScale, newScale};

      // 下端が地面に埋まらないように、スケールに合わせて持ち上げる
      // ・元々の下端 = bulletChargeBaseHeight_ - enemyHeightOffset_
      // ・現在の半分の高さ = enemyHeightOffset_ * (newScale / baseScaleY)
      float bottomY = bulletChargeBaseHeight_ - enemyHeightOffset_;
      float scaledHalfHeight = enemyHeightOffset_ * (newScale / baseScaleY);
      transform_.translate.y = bottomY + scaledHalfHeight;
    } else {
      // ここに来たフレームでチャージ完了 → 弾発射
      bulletChargeProgress_ = 0.0f;

      if (enemyBullet_) {
        // ★ 弾の発射位置は見た目とは切り離して「Y=0固定」
        Vector3 bulletOrigin = transform_.translate;
        bulletOrigin.y = 0.0f; // ここだけ固定値にする

        if (phase_ == EnemyPhase::Phase2) {
          // フェーズ2：3発同時発射
          enemyBullet_->SpawnBulletSpread(
              bulletOrigin,           // Y=0 から出す
              playerPos,              // 狙い先
              3,                      // 発射数
              bulletSpreadAngleRad_); // 左右の開き角
        } else {
          // フェーズ1：1発だけ
          enemyBullet_->SpawnBulletAimed(bulletOrigin, // Y=0 から出す
                                         playerPos);
        }
      }

      // スケールと高さを元に戻す（膨らみ演出リセット）
      transform_.scale = bulletChargeBaseScale_;
      transform_.translate.y = bulletChargeBaseHeight_;

      // 次の行動待ちへ
      state_ = EnemyState::Idle;
      ResetActionTimer();
    }
    break;
  }

  case EnemyState::BurrowPreDive: {
    // 潜る前のもぞもぞ（その場で揺れる）
    stateTimer_ -= deltaTime;
    burrowPhase_ += deltaTime * burrowWobbleFrequency_;
    float wobble = std::sin(burrowPhase_) * burrowWobbleAmplitude_;
    transform_.translate.y = groundY_ + wobble;

    if (stateTimer_ <= 0.0f) {
      // ここで潜りきって地中へ → 見えなくなる
      isBurrowing_ = true;

      // 次に出てくる位置を決める（ワープは見えない間にしてしまう）
      Vector3 newPos = GetRandomReappearPosition(playerPos);
      transform_.translate.x = newPos.x;
      transform_.translate.z = newPos.z;
      groundY_ = newPos.y + enemyHeightOffset_;
      transform_.translate.y = groundY_ - 1.0f; // 少し下げて地中にいるイメージ

      state_ = EnemyState::BurrowHidden;
      stateTimer_ = burrowHiddenDuration_;
    }
    break;
  }

  case EnemyState::BurrowHidden: {
    // 地中にいる状態（見えない＆当たり判定も取らない）
    stateTimer_ -= deltaTime;
    if (stateTimer_ <= 0.0f) {
      // 出現フェーズへ（出現先の地面の位置で揺れる）
      isBurrowing_ = false; // ここからは見える
      burrowPhase_ = 0.0f;
      transform_.translate.y = groundY_;

      state_ = EnemyState::BurrowEmerge;
      stateTimer_ = burrowEmergeDuration_;
    }
    break;
  }

  case EnemyState::BurrowEmerge: {
    // 出てくるときのもぞもぞ
    stateTimer_ -= deltaTime;
    burrowPhase_ += deltaTime * burrowWobbleFrequency_;
    float wobble = std::sin(burrowPhase_) * burrowWobbleAmplitude_;
    transform_.translate.y = groundY_ + wobble;

    if (stateTimer_ <= 0.0f) {
      // もぞもぞ完了で通常状態へ戻る
      transform_.translate.y = groundY_;
      state_ = EnemyState::Idle;
      ResetActionTimer();
    }
    break;
  }

  case EnemyState::ChargeBack: {
    // 突進の予備動作（後ろに下がる）
    stateTimer_ -= deltaTime;

    // 後退方向 = 突進方向の逆
    Vector3 back;
    back.x = -dashDirection_.x;
    back.y = 0.0f;
    back.z = -dashDirection_.z;

    float move = 0.0f;
    if (dashChargeBackTime_ > 0.0f) {
      move = (dashBackDistance_ / dashChargeBackTime_) * deltaTime;
    }

    transform_.translate.x += back.x * move;
    transform_.translate.z += back.z * move;

    if (stateTimer_ <= 0.0f) {
      state_ = EnemyState::DashForward;
      dashMoved_ = 0.0f;
    }
    break;
  }

  case EnemyState::DashForward: {
    // 高速突進中
    float move = dashSpeed_ * deltaTime;

    transform_.translate.x += dashDirection_.x * move;
    transform_.translate.z += dashDirection_.z * move;

    dashMoved_ += move;

    // ステージ外に出たかチェック（この時点ではワープせずフラグだけ立てる）
    {
      const float outOfBoundsMargin = 0.5f;
      float distFromCenter = DistanceXZ(transform_.translate, stageCenter_);
      if (distFromCenter > stageRadius_ + outOfBoundsMargin) {
        // 次の行動は確定で潜り移動させたいのでフラグを立てる
        forceBurrowOnNextAction_ = true;
      }
    }

    // 一定距離進んだら終了
    if (dashMoved_ >= dashDistance_) {
      state_ = EnemyState::Idle;
      ResetActionTimer();
    }

    break;
  }

  default:
    break;
  }

  // --- ここから敵側で持つ当たり判定（弾 vs 壁 / 敵 vs 壁） ---

  // 突進中の敵本体 vs 壁 壁だけ壊す
  if (enemyWall_) {
    if (state_ == EnemyState::DashForward) {
      int hitWallIndex = enemyWall_->CheckCollisionCircle(transform_.translate,
                                                          enemyBodyRadius_);
      if (hitWallIndex >= 0) {
        enemyWall_->OnEnemyHitWall(hitWallIndex);
      }
    }
  }

  // 弾 vs 壁：両方消す
  if (enemyWall_ && enemyBullet_) {
    enemyBullet_->ResolveBulletWallCollision(*enemyWall_);
  }

#ifdef _DEBUG
  // ImGui デバッグ
  ImGui::Begin("Enemy");
  ImGui::Text("Phase: %d", static_cast<int>(phase_) + 1);
  ImGui::Text("State: %d", static_cast<int>(state_));
  ImGui::Text("ActionTimer: %.2f", actionTimer_);
  ImGui::Text("BulletCharge: %.2f", bulletChargeProgress_);
  ImGui::Text("hp: %d / %d", hp_, maxHp_);
  ImGui::End();

  ImGui::Begin("Enemy Parameters");

  // 行動オン/オフ
  ImGui::Text("Action Enable");
  ImGui::Checkbox("Enable Wall Attack", &enableWallAttack_);
  ImGui::Checkbox("Enable Bullet Attack", &enableBulletAttack_);
  ImGui::Checkbox("Enable Burrow Move", &enableBurrowMove_);
  ImGui::Checkbox("Enable Dash Attack", &enableDashAttack_);

  // 敵サイズ関連
  ImGui::Separator();
  ImGui::Text("Enemy Body");
  ImGui::SliderFloat("Body Radius", &enemyBodyRadius_, 0.1f, 5.0f);
  ImGui::SliderFloat("Height Offset", &enemyHeightOffset_, 0.0f, 5.0f);

  // 壁攻撃
  if (enableWallAttack_) {
    ImGui::Separator();
    ImGui::Text("Wall Attack");
    ImGui::SliderFloat("Wall_Height", &wallRiseHeight_, 0.0f, 10.0f);
    ImGui::SliderFloat("Wall_UpTime", &wallRiseDuration_, 0.1f, 2.0f);
    ImGui::SliderFloat("Wall_DownTime", &wallDropDuration_, 0.05f, 1.0f);
  }

  // 弾攻撃
  if (enableBulletAttack_) {
    ImGui::Separator();
    ImGui::Text("Bullet Attack");
    ImGui::SliderFloat("Bullet_ChargeTime", &bulletChargeDuration_, 0.1f, 3.0f);
    ImGui::SliderFloat("Bullet_SwellAmount", &bulletChargeScaleAmount_, 0.0f,
                       0.8f);
    ImGui::SliderFloat("Bullet_SpreadAngle", &bulletSpreadAngleRad_, 0.05f,
                       0.6f);
  }

  // 潜り移動
  if (enableBurrowMove_) {
    ImGui::Separator();
    ImGui::Text("Dive Move");
    ImGui::SliderFloat("Dive_DownTime", &burrowPreDuration_, 0.1f, 2.0f);
    ImGui::SliderFloat("Dive_HiddenTime", &burrowHiddenDuration_, 0.1f, 3.0f);
    ImGui::SliderFloat("Dive_UpTime", &burrowEmergeDuration_, 0.1f, 2.0f);

    ImGui::SliderFloat("Dive_WobbleAmp", &burrowWobbleAmplitude_, 0.0f, 2.0f);
    ImGui::SliderFloat("Dive_WobbleFreq", &burrowWobbleFrequency_, 1.0f, 20.0f);
  }

  // 突進攻撃
  if (enableDashAttack_) {
    ImGui::Separator();
    ImGui::Text("Dash Attack");
    ImGui::SliderFloat("Dash_ChargeTime", &dashChargeBackTime_, 0.1f, 1.5f);
    ImGui::SliderFloat("Dash_BackDist", &dashBackDistance_, 0.1f, 3.0f);
    ImGui::SliderFloat("Dash_Speed", &dashSpeed_, 5.0f, 40.0f);
    ImGui::SliderFloat("Dash_Distance", &dashDistance_, 3.0f, 20.0f);
  }

  // 行動間隔
  ImGui::Separator();
  ImGui::Text("Action Interval");
  ImGui::DragFloat("Interval_Min", &actionIntervalMin_, 0.1f);
  ImGui::DragFloat("Interval_Max", &actionIntervalMax_, 0.1f);
  if (actionIntervalMin_ > actionIntervalMax_) {
    actionIntervalMin_ = actionIntervalMax_;
  }

  // 行動重み（近・中・遠）
  ImGui::Separator();
  ImGui::Text("Action Weights (Near / Mid / Far)");

  ImGui::Text("Near (dist < 4.0)");
  ImGui::DragFloat("N_Bullet", &weightsNear_.bullet, 1.0f, 0.0f, 100.0f);
  ImGui::DragFloat("N_Wall", &weightsNear_.wall, 1.0f, 0.0f, 100.0f);
  ImGui::DragFloat("N_Burrow", &weightsNear_.burrow, 1.0f, 0.0f, 100.0f);
  ImGui::DragFloat("N_Dash", &weightsNear_.dash, 1.0f, 0.0f, 100.0f);

  ImGui::Separator();
  ImGui::Text("Mid (4.0 <= dist < 8.0)");
  ImGui::DragFloat("M_Bullet", &weightsMid_.bullet, 1.0f, 0.0f, 100.0f);
  ImGui::DragFloat("M_Wall", &weightsMid_.wall, 1.0f, 0.0f, 100.0f);
  ImGui::DragFloat("M_Burrow", &weightsMid_.burrow, 1.0f, 0.0f, 100.0f);
  ImGui::DragFloat("M_Dash", &weightsMid_.dash, 1.0f, 0.0f, 100.0f);

  ImGui::Separator();
  ImGui::Text("Far (dist >= 8.0)");
  ImGui::DragFloat("F_Bullet", &weightsFar_.bullet, 1.0f, 0.0f, 100.0f);
  ImGui::DragFloat("F_Wall", &weightsFar_.wall, 1.0f, 0.0f, 100.0f);
  ImGui::DragFloat("F_Burrow", &weightsFar_.burrow, 1.0f, 0.0f, 100.0f);
  ImGui::DragFloat("F_Dash", &weightsFar_.dash, 1.0f, 0.0f, 100.0f);

  ImGui::End();
#endif // _DEBUG

  if (model_) {
    model_->SetPosition(transform_.translate);
    model_->SetScale(transform_.scale);
    model_->Update();
  }
}

void Enemy::Draw() {
  // 潜り中（BurrowHidden）の間だけ見えなくする
  if (!isBurrowing_) {
    if (model_) {
      model_->Draw();
    }
  }

  // 壁の描画
  if (enemyWall_) {
    enemyWall_->Draw();
  }

  // 弾の描画
  if (enemyBullet_) {
    enemyBullet_->Draw();
  }
}

void Enemy::ResetActionTimer() {
  actionTimer_ = RandomRangeEnemy(actionIntervalMin_, actionIntervalMax_);
}

Vector3 Enemy::GetRandomReappearPosition(const Vector3 &playerPos) const {
  float innerRadius = 5.0f;
  float outerRadius = stageRadius_ - 3.0f;
  float minPlayerDistance = 8.0f;

  Vector3 result = transform_.translate;
  const int kMaxTry = 20;

  for (int i = 0; i < kMaxTry; ++i) {
    float angle = RandomRangeEnemy(0.0f, 6.28318530718f);
    float radius = RandomRangeEnemy(innerRadius, outerRadius);

    Vector3 pos;
    pos.x = stageCenter_.x + std::cos(angle) * radius;
    pos.y = stageCenter_.y;
    pos.z = stageCenter_.z + std::sin(angle) * radius;

    // プレイヤーから近すぎない
    if (DistanceXZ(pos, playerPos) < minPlayerDistance) {
      continue;
    }

    // 壁の中に出ないかチェック（少し余裕 margin を取る）
    if (IsInsideAnyWall(pos, 0.3f)) {
      continue;
    }

    result = pos;
    return result;
  }

  // うまく見つからなかったら、最後に試した位置か現在位置にする
  return result;
}

bool Enemy::IsInsideAnyWall(const Vector3 &pos, float margin) const {
  if (!enemyWall_) {
    return false;
  }

  const auto &walls = enemyWall_->GetWalls();

  for (const auto &w : walls) {
    if (!w.active) {
      continue;
    }

    float dx = pos.x - w.position.x;
    float dz = pos.z - w.position.z;

    float halfX = w.halfSizeX + margin;
    float halfZ = w.halfSizeZ + margin;

    // AABB 内に入っているかどうか
    if (std::abs(dx) <= halfX && std::abs(dz) <= halfZ) {
      return true;
    }
  }

  return false;
}

// --------------------------------------
// プレイヤーからのダメージ関連
// --------------------------------------

void Enemy::ApplyDamageFromPlayer(int damage) {
  if (damage <= 0) {
    return;
  }

  // 突進中はダメージ無効
  if (state_ == EnemyState::DashForward) {
    return;
  }

  hp_ -= damage;
  if (hp_ < 0) {
    hp_ = 0;
  }

  // フェーズ1で HP が尽きたらフェーズ2へ移行して全快
  if (phase_ == EnemyPhase::Phase1 && hp_ <= 0) {
    EnterPhase2();
  }
}

// --------------------------------------
// フェーズ2突入処理
// --------------------------------------
void Enemy::EnterPhase2() {
  phase_ = EnemyPhase::Phase2;

  // HP を全回復
  hp_ = maxHp_;

  // 行動間隔を 2〜4秒 → 1〜2秒へ
  actionIntervalMin_ = 1.0f;
  actionIntervalMax_ = 2.0f;

  // 突進を強化（ため短縮・速度アップ・距離延長）
  dashChargeBackTime_ *= 0.5f; // 予備動作を半分
  dashSpeed_ *= 1.5f;          // 速度 1.5 倍
  dashDistance_ *= 1.5f;       // 距離 1.5 倍

  // それまでの行動状態をリセット
  state_ = EnemyState::Idle;
  stateTimer_ = 0.0f;
  isBurrowing_ = false;
  bulletChargeProgress_ = 0.0f;
  burrowPhase_ = 0.0f;

  // 次の行動までの時間を再設定
  ResetActionTimer();
}

// --------------------------------------
// 突進の強制停止
// --------------------------------------

void Enemy::ForceStopDash() {
  if (state_ == EnemyState::DashForward || state_ == EnemyState::ChargeBack) {
    state_ = EnemyState::Idle;
    stateTimer_ = 0.0f;
    dashMoved_ = 0.0f;
    ResetActionTimer();
  }
}
