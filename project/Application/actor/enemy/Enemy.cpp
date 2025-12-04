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
  model_->Initialize(camera_, "player.obj");

  transform_.translate = spawnPos;
  transform_.scale = {1.0f, 1.0f, 1.0f};

  enemyWall_ = wallManager;
  enemyBullet_ = bulletManager;
  hp_ = 100;

  // ステージ情報はとりあえず固定値にしておく
  stageCenter_ = Vector3{0.0f, 0.0f, 0.0f};
  stageRadius_ = stageRadius;

  // 地面の高さ（上下の基準）
  groundY_ = spawnPos.y;

  isBurrowing_ = false;
  burrowTimer_ = 0.0f;

  lastHitResult_ = EnemyPlayerHitResult{};

  // 行動状態関連の初期化
  state_ = EnemyState::Idle;
  stateTimer_ = 0.0f;
  bulletChargeProgress_ = 0.0f;
  burrowPhase_ = 0.0f;

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
      bool canWall = (enemyWall_ != nullptr);
      bool canBullet = (enemyBullet_ != nullptr);

      // 行動候補をビットフラグ的に考える
      // 0: 壁, 1: 弾, 2: 潜り
      int actionChoices[3];
      int actionCount = 0;

      if (canWall) {
        actionChoices[actionCount++] = 0;
      }
      if (canBullet) {
        actionChoices[actionCount++] = 1;
      }
      // 潜り行動は常に候補に入れておく
      actionChoices[actionCount++] = 2;

      // どれか1つをランダムに選ぶ
      int index = RandomIntEnemy(0, actionCount - 1);
      int action = actionChoices[index];

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
        }
        break;
      case 2: // 潜り移動：もぞもぞ → 地中 → もぞもぞ
        state_ = EnemyState::BurrowPreDive;
        stateTimer_ = burrowPreDuration_;
        burrowPhase_ = 0.0f;
        isBurrowing_ = false; // もぞもぞ中は見えている
        groundY_ = transform_.translate.y;
        break;
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
        enemyWall_->SpawnWalls(transform_.translate);
      }
      // 次の行動待ちへ
      state_ = EnemyState::Idle;
      ResetActionTimer();
    }
    break;
  }

  case EnemyState::BulletCharge: {
    // 弾チャージ中（赤くなっていくイメージ）
    stateTimer_ -= deltaTime;
    float t = 1.0f - (stateTimer_ / bulletChargeDuration_);
    if (t < 0.0f) {
      t = 0.0f;
    }
    if (t > 1.0f) {
      t = 1.0f;
    }
    bulletChargeProgress_ = t;

    if (stateTimer_ <= 0.0f) {
      // チャージ完了で弾発射
      if (enemyBullet_) {
        enemyBullet_->SpawnBulletAimed(transform_.translate, playerPos);
      }
      bulletChargeProgress_ = 0.0f;

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
      groundY_ = newPos.y;
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

  default:
    break;
  }

#ifdef _DEBUG
  // ImGui デバッグ
  ImGui::Begin("Enemy");
  ImGui::Text("State: %d", static_cast<int>(state_));
  ImGui::Text("ActionTimer: %.2f", actionTimer_);
  ImGui::Text("BulletCharge: %.2f", bulletChargeProgress_);
  ImGui::Text("hp: %d", hp_);

  ImGui::End();

  ImGui::Begin("Enemy Parameters");

  // 壁攻撃
  ImGui::Text("Wall Attack");
  ImGui::SliderFloat("Height", &wallRiseHeight_, 0.0f, 10.0f);
  ImGui::SliderFloat("UpTime", &wallRiseDuration_, 0.1f, 2.0f);
  ImGui::SliderFloat("DownTime", &wallDropDuration_, 0.05f, 1.0f);

  // 弾攻撃
  ImGui::Separator();
  ImGui::Text("Bullet Attack");
  ImGui::SliderFloat("ChargeTime", &bulletChargeDuration_, 0.1f, 3.0f);

  // 潜り移動
  ImGui::Separator();
  ImGui::Text("Dive Move");
  ImGui::SliderFloat("DownTime", &burrowPreDuration_, 0.1f, 2.0f);
  ImGui::SliderFloat("HiddenTime", &burrowHiddenDuration_, 0.1f, 3.0f);
  ImGui::SliderFloat("UpTime", &burrowEmergeDuration_, 0.1f, 2.0f);

  ImGui::SliderFloat("WobbleAmp", &burrowWobbleAmplitude_, 0.0f, 2.0f);
  ImGui::SliderFloat("WobbleFreq", &burrowWobbleFrequency_, 1.0f, 20.0f);

  // 攻撃間隔
  ImGui::Text("actionInterval");
  ImGui::DragFloat("Min", &actionIntervalMin_,
                   0.1f); // actionInterval(行動最小間隔)
  ImGui::DragFloat("Max", &actionIntervalMax_,
                   0.1f); // actionInterval(行動最大間隔)
  if (actionIntervalMin_ > actionIntervalMax_)
    actionIntervalMin_ = actionIntervalMax_;

  ImGui::End();
#endif // _DEBUG

  // 将来、敵を横移動させたくなったらここで transform_.translate を変更する

  if (model_) {
    model_->SetPosition(transform_.translate);
    // bulletChargeProgress_ を使って色を変えたいときは、
    // ObjClass にカラーパラメータを持たせてもらってここで渡す想定。
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
// ここからプレイヤーとの当たり判定関連
// --------------------------------------

void Enemy::CheckCollisionsWithPlayer(Player *player) {
  // 毎フレーム最初に結果をリセット
  lastHitResult_ = EnemyPlayerHitResult{};

  // 壁との当たり判定（プレイヤー円 vs 壁）
  if (enemyWall_) {
    int wallIndex = enemyWall_->CheckCollisionCircle(player->GetPosition(),
                                                     player->GetRadius());
    if (wallIndex >= 0) {
      lastHitResult_.hitWall = true;
      lastHitResult_.wallIndex = wallIndex;

      // 壁側に「プレイヤーが当たった」ことを通知
      enemyWall_->OnPlayerHitWall(wallIndex);
    }
  }

  // 弾との当たり判定（プレイヤー円 vs 弾）
  if (enemyBullet_) {
    int bulletIndex = enemyBullet_->CheckCollisionCircle(player->GetPosition(),
                                                         player->GetRadius());
    if (bulletIndex >= 0) {
      lastHitResult_.hitBullet = true;
      lastHitResult_.bulletIndex = bulletIndex;

      // 弾側に「当たった」ことを通知（弾を消すなど）
      enemyBullet_->OnHitBullet(bulletIndex);
    }
  }

  // 敵本体との当たり判定（プレイヤー円 vs 敵円）
  // 潜って見えない間（BurrowHidden）は当たり判定を取らない
  //if (!isBurrowing_) {
  //  float dx = player->GetPosition().x - transform_.translate.x;
  //  float dz = player->GetPosition().z - transform_.translate.z;
  //  float distSq = dx * dx + dz * dz;
  //  float r = player->GetRadius() + enemyBodyRadius_;
  //  float rSq = r * r;

  //  if (distSq <= rSq) {
  //    lastHitResult_.hitEnemyBody = true;

  //    player->ResetRockCount();        // プレイヤー側で岩をリセット
  //    hp_ -= player->GetAttackPower(); // 敵にダメージを与える
  //  }
  //}
}

// --------------------------------------
// プレイヤーからのダメージ関連
// --------------------------------------

void Enemy::ApplyDamageFromPlayer(int damage) {
  if (damage <= 0) {
    return;
  }

  hp_ -= damage;
  if (hp_ < 0) {
    hp_ = 0;
  }

  // 将来的にノックバックやフェーズ移行などはここで書く
  // if (IsDead()) { ... }
}
