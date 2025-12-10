#include "RockManager.h"

#include "3D/Region.h"
#include "Application/camera/Camera.h"
#include "actor/player/Player.h"
#include "contents/GameFunction.h"
#include "contents/Quaternion.h"
#include "engine/IrufemiEngine.h"
#include "stage/field/Field.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace {

/// <summary>
/// 正規化
/// </summary>
/// <returns></returns>
Vector3 NormalizeVec(const Vector3 &v) {
  float lenSq = v.x * v.x + v.y * v.y + v.z * v.z;
  if (lenSq <= 0.0f) {
    return {0.0f, 0.0f, 0.0f};
  }
  float invLen = 1.0f / std::sqrt(lenSq);
  return {v.x * invLen, v.y * invLen, v.z * invLen};
}

/// <summary>
/// 内積を返す
/// </summary>
/// <returns></returns>
float DotVec(const Vector3 &a, const Vector3 &b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

/// <summary>
/// ローカルY軸周りに回転させる
/// </summary>
/// <returns></returns>
Vector3 RotateAroundYLocal(const Vector3 &dir, float angleRad) {
  float c = std::cos(angleRad);
  float s = std::sin(angleRad);
  Vector3 r;
  r.x = dir.x * c - dir.z * s;
  r.y = dir.y;
  r.z = dir.x * s + dir.z * c;
  return r;
}

/// <summary>
/// クロス積を返す
/// </summary>
Vector3 CrossVec(const Vector3 &a, const Vector3 &b) {
  return {
      a.y * b.z - a.z * b.y,
      a.z * b.x - a.x * b.z,
      a.x * b.y - a.y * b.x,
  };
}

} // namespace

void RockManager::Initialize(Camera *camera) {
  // Rock用 Region を生成
  rockRegion_ = std::make_unique<Region>();

  // モデルを読み込む
  rockRegion_->Initialize(camera, "block.obj");
}

void RockManager::AddRock(const Vector3 &pos, float radius) {
  // 上限を超える場合は追加しない
  if (rocks_.size() >= maxAlive_) {
    return;
  }

  // 指定位置と半径で岩を追加する
  rocks_.emplace_back(pos, radius);
}

void RockManager::SetSpawnArea(const Vector3 &minPos, const Vector3 &maxPos) {
  spawnMin_ = minPos;
  spawnMax_ = maxPos;
}

void RockManager::Update(Player *player) {
  const float dt = 1.0f / 60.0f; // 1フレーム時間

  // 岩個別の更新とスポーン処理
  UpdateRocks(dt);
  AutoSpawn(dt);

  //  プレイヤーがいなければ何もしない
  if (!player) {
    return;
  }

  lastPlayer_ = player;

  // プレイヤー情報を取得
  const Vector3 &pPos = player->GetPosition();
  const float pRadius = player->GetRadius();

  // すべての岩とプレイヤーの当たり判定
  for (auto &rock : rocks_) {

    if (rock.isAttached_) {
      // ローカル方向をプレイヤー姿勢に基づいて回し、公転の動きをさせる
      Vector3 rotated = player->RotateLocalDir(rock.localDir_);
      rock.position_ = pPos + rotated * rock.distanceFromPlayer_;

      // プレイヤー姿勢 × ローカル回転をしてワールド姿勢を取得する
      const Quaternion &qPlayer = player->GetRotation();
      Quaternion qWorld = Multiply(qPlayer, rock.localRotation_);

      // クォータニオンからオイラー角に変換
      rock.rotate_ = QuaternionToEuler(qWorld);

      continue;
    }

    // 生存していない岩はスキップ
    if (!rock.isAlive_) {
      continue;
    }

    if (rock.isDropped_) {
      continue;
    }

    // 球 vs 球の当たり判定
    if (GameFunction::IsHitSphere(pPos, pRadius, rock.position_,
                                  rock.radius_)) {

      // 岩をプレイヤーにくっついた状態にする
      rock.isAttached_ = true;

      // フィールド上からは消す
      rock.Kill();

      // プレイヤーに岩を1つ追加
      player->AddRock(1);

      // プレイヤーから見た岩の方向（ワールド）
      Vector3 diff = rock.position_ - pPos;
      Vector3 worldDir = NormalizeVec(diff);

      // ローカル方向の初期候補
      Vector3 candidateLocalDir = player->WorldDirToLocal(worldDir);

      // 最小角度(ほかの岩と重ならないように)
      const float minAngleRad = 0.5f;
      const float cosMinAngle = std::cos(minAngleRad);

      // 近すぎたときの調整用パラメータ
      const float adjustStepRad = 0.4f;
      const int maxTries = 12;

      // 他の岩と近すぎたら離す
      for (int i = 0; i < maxTries; ++i) {
        bool separated = true;

        // すでにくっついている岩との角度
        for (const auto &other : rocks_) {
          if (!other.isAttached_) {
            continue;
          }

          float dot = DotVec(candidateLocalDir, other.localDir_);
          if (dot > cosMinAngle) {
            // 角度が近すぎる(岩同士が重なっている)
            separated = false;
            break;
          }
        }

        if (separated) {
          break;
        }

        // 近すぎたので、ローカルY軸まわりに少し回転させて再チェック
        candidateLocalDir =
            RotateAroundYLocal(candidateLocalDir, adjustStepRad);
      }

      // ローカル方向とプレイヤー中心からの距離を保存する
      rock.localDir_ = candidateLocalDir;
      rock.distanceFromPlayer_ = pRadius + rock.radius_;

      // 岩の +Y をプレイヤー中心方向へ向けるための回転を計算する
      Vector3 nWorld = worldDir;
      Vector3 up{0.0f, 1.0f, 0.0f};

      float dot = DotVec(up, nWorld);
      const float EPS = 1e-4f;

      Quaternion qAttach; // ワールド空間での岩の姿勢(最終的にオイラー角に戻す)

      if (dot > 1.0f - EPS) {
        // ほぼ同一方向の場合は回転不要
        qAttach = {0.0f, 0.0f, 0.0f, 1.0f};
      } else if (dot < -1.0f + EPS) {
        // 反対方向の場合は X 軸まわりに180度回転
        qAttach = FromAxisAngle({1.0f, 0.0f, 0.0f}, 3.14159265f);
      } else {
        // up を nWorld に向ける回転
        Vector3 axis = CrossVec(up, nWorld);
        axis = NormalizeVec(axis);
        float angle = std::acos(dot);
        qAttach = FromAxisAngle(axis, angle);
      }

      // プレイヤーの姿勢クォータニオン
      Quaternion qPlayer = player->GetRotation();

      // 逆回転(クォータニオン)
      Quaternion qPlayerInv{-qPlayer.x, -qPlayer.y, -qPlayer.z, qPlayer.w};

      // プレイヤー基準のローカル回転として保存
      rock.localRotation_ = Multiply(qPlayerInv, qAttach);
    }
  }
}

void RockManager::UpdateRocks(float deltaTime) {

    // フィールド半径（場外判定用）
    float fieldRadius = 0.0f;
    if (field_) {
        fieldRadius = field_->GetRadius(); // 円フィールドの半径
    }

  // 岩の更新処理
  for (auto &r : rocks_) {
    if (!r.isAlive_)
      continue;
    r.Update(deltaTime);

    // === 場外チェック ===
    // プレイヤーにくっついていない & スポーン中でない岩だけ判定
    if (fieldRadius > 0.0f && !r.isAttached_ && !r.isSpawning_) {

        float x = r.position_.x;
        float z = r.position_.z;
        float distSq = x * x + z * z;
        float limitSq = fieldRadius * fieldRadius;

        // フィールドの外側に出たら縮小開始
        if (distSq > limitSq) {
            if (!r.isShrinking_) {
                r.isShrinking_ = true;
                r.shrinkTimer_ = 0.0f;
                r.shrinkStartRadius_ = r.radius_;
            }
        }
    }
  }

  // 死亡しているかつ、プレイヤーにくっついていない岩を削除
  rocks_.erase(std::remove_if(
                   rocks_.begin(), rocks_.end(),
                   [](const Rock &r) { return !r.isAlive_ && !r.isAttached_; }),
               rocks_.end());
}

void RockManager::AutoSpawn(float deltaTime) {

  // スポーンタイマーを進める
  spawnTimer_ += deltaTime;

  // 生存している岩を数える
  size_t alive = 0;
  for (const auto &r : rocks_) {
    if (r.isAlive_) {
      ++alive;
    }
  }

  // 上限を超えている場合はスポーンしない
  if (alive >= maxAlive_) {
    return;
  }
  OutputDebugStringA((std::to_string(alive) + "\n").c_str());

  // インターバル経過したらスポーン
  if (spawnTimer_ >= spawnInterval_) {
    spawnTimer_ = 0.0f;
    SpawnRandomRock();
  }
}

void RockManager::SpawnRandomRock() {
  static std::mt19937 rng{std::random_device{}()};

  std::uniform_real_distribution<float> distX(spawnMin_.x, spawnMax_.x);
  std::uniform_real_distribution<float> distZ(spawnMin_.z, spawnMax_.z);

  // ランダム位置に岩を生成する
  Vector3 pos{};

  if (field_) {
    // ★ Field が設定されている場合 → 丸フィールド内のランダム位置
    //    y の高さは元の spawnMin_.y を流用
    pos = field_->GetRandomPointInField();
  } else {
    // ★ Field がない場合 → 既存の矩形スポーンにフォールバック
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> distX(spawnMin_.x, spawnMax_.x);
    std::uniform_real_distribution<float> distZ(spawnMin_.z, spawnMax_.z);

    pos.x = distX(rng);
    pos.y = spawnMin_.y;
    pos.z = distZ(rng);
  }

  float radius = 0.5f;
  AddRock(pos, radius);
}

void RockManager::ResetAttachedRocks() {

  for (auto &rock : rocks_) {
    if (rock.isAttached_) {
      rock.isAttached_ = false;
    }
  }
}

void RockManager::HalveAttachedRocks(int numToDetach) {

  if (numToDetach <= 0) {
    return;
  }

  // いまプレイヤーに纏っている岩のインデックスを集める
  std::vector<int> attachedIndices;
  attachedIndices.reserve(rocks_.size());
  for (int i = 0; i < static_cast<int>(rocks_.size()); ++i) {
    if (rocks_[i].isAttached_) {
      attachedIndices.push_back(i);
    }
  }

  if (attachedIndices.empty()) {
    return;
  }

  // distanceFromPlayer_ が大きい順（外側順）にソート
  std::sort(
      attachedIndices.begin(), attachedIndices.end(), [this](int a, int b) {
        return rocks_[a].distanceFromPlayer_ > rocks_[b].distanceFromPlayer_;
      });
  int detachCount =
      (std::min)(numToDetach, static_cast<int>(attachedIndices.size()));

  // 「フィールドに飛ばす岩」の数（失った分の半分）
  int spawnCount = detachCount / 2;
  if (detachCount > 0 && spawnCount == 0) {
    spawnCount = 1; // 1個でも失ったら最低1個は飛ばす
  }

  // 飛ばす中心（プレイヤー位置）
  Vector3 center{0.0f, 0.0f, 0.0f};
  if (lastPlayer_) {
    center = lastPlayer_->GetPosition();
  }

  // 乱数準備
  static std::mt19937 rng{std::random_device{}()};

  // 水平方向の飛び散り角度
  std::uniform_real_distribution<float> distAngle(0.0f, 6.0f * 3.14159265f);

  // 水平スピード
  std::uniform_real_distribution<float> distSpeed(10.0f, 20.0f);

  // 上方向初速
  std::uniform_real_distribution<float> distUp(10.0f, 15.0f);

  for (int i = 0; i < detachCount; ++i) {
    int idx = attachedIndices[i];
    if (idx < 0 || idx >= static_cast<int>(rocks_.size())) {
      continue;
    }

    Rock &rock = rocks_[idx];

    // 纏い状態」解除
    rock.isAttached_ = false;

    if (i < spawnCount && lastPlayer_) {

      // プレイヤー位置から飛ばす
      rock.position_ = center;

      // 着地地点
      rock.spawnEndY_ = 0.0f;

      // ランダム方向（XZ 平面）
      float angle = distAngle(rng);
      Vector3 dirXZ{std::cos(angle), 0.0f, std::sin(angle)};

      float speed = distSpeed(rng);
      float upSpeed = distUp(rng);

      // 初速セット
      rock.velocity_.x = dirXZ.x * speed;
      rock.velocity_.z = dirXZ.z * speed;
      rock.velocity_.y = upSpeed;

      rock.isAlive_ = true;   // フィールド岩として有効
      rock.isDropped_ = true; // 飛び散り中

      // スポーン演出は今回は使わない
      rock.isSpawning_ = false;

    } else {
      rock.isAlive_ =
          false; // isAttached_ も false なので、次の UpdateRocks で消える
      rock.isDropped_ = false;
    }
  }
}

std::vector<int> RockManager::SelectDetachedRocks(int lostCount) {
  std::vector<int> attachedIndices;
  attachedIndices.reserve(rocks_.size());

  // いまプレイヤーに纏っている岩のインデックスを集める
  for (int i = 0; i < static_cast<int>(rocks_.size()); ++i) {
    if (rocks_[i].isAttached_) {
      attachedIndices.push_back(i);
    }
  }

  if (attachedIndices.empty() || lostCount <= 0) {
    return {};
  }

  // distanceFromPlayer_ が大きい順（外側順）にソート
  std::sort(
      attachedIndices.begin(), attachedIndices.end(), [this](int a, int b) {
        return rocks_[a].distanceFromPlayer_ > rocks_[b].distanceFromPlayer_;
      });

  int detachCount =
      (std::min)(lostCount, static_cast<int>(attachedIndices.size()));

  std::vector<int> result;
  result.reserve(detachCount);

  for (int i = 0; i < detachCount; ++i) {
    result.push_back(attachedIndices[i]);
  }

  return result;
}

void RockManager::SpawnDroppedRocks(const std::vector<int> &detachedList,
                                    int spawnCount, const Vector3 &playerPos,
                                    const Vector3 &knockbackDir) {
  if (detachedList.empty()) {
    return;
  }

  // spawnCount が detachedList を超えないようにクランプ
  spawnCount = (std::min)(spawnCount, static_cast<int>(detachedList.size()));

  // ノックバック方向（XZ平面）を正規化
  Vector3 baseDir{knockbackDir.x, 0.0f, knockbackDir.z};
  baseDir = NormalizeVec(baseDir);
  if (baseDir.x == 0.0f && baseDir.y == 0.0f && baseDir.z == 0.0f) {
    // ノックバック方向が無効なら前方向をデフォルトにする
    baseDir = {0.0f, 0.0f, 1.0f};
  }

  // 乱数準備
  static std::mt19937 rng{std::random_device{}()};

  // 前方円錐の広がり（ラジアン）
  const float maxSpreadRad = 3.14159265f / 6.0f;
  std::uniform_real_distribution<float> distAngle(-maxSpreadRad, maxSpreadRad);

  // 水平速度の大きさ
  const float minSpeed = 10.0f;
  const float maxSpeed = 20.0f;
  std::uniform_real_distribution<float> distSpeed(minSpeed, maxSpeed);

  // 上向き初速
  const float minUp = 10.0f;
  const float maxUp = 16.0f;
  std::uniform_real_distribution<float> distUp(minUp, maxUp);

  for (int i = 0; i < static_cast<int>(detachedList.size()); ++i) {
    int idx = detachedList[i];
    if (idx < 0 || idx >= static_cast<int>(rocks_.size())) {
      continue;
    }

    Rock &rock = rocks_[idx];

    // まず纏っていない状態にする
    rock.isAttached_ = false;

    if (i < spawnCount) {
      // プレイヤー位置から飛ばし始める
      rock.position_ = playerPos;

      // 着地する高さを決める
      rock.spawnEndY_ = 0.0f;

      // XZ 方向：ノックバック方向を中心に少し左右にばらけさせる
      float angle = distAngle(rng);
      Vector3 dirXZ = RotateAroundYLocal(baseDir, angle);
      dirXZ = NormalizeVec(dirXZ);

      float speed = distSpeed(rng);
      float upSpeed = distUp(rng);

      // 初速セット
      rock.velocity_.x = dirXZ.x * speed;
      rock.velocity_.z = dirXZ.z * speed;
      rock.velocity_.y = upSpeed;

      // 状態フラグ
      rock.isAlive_ = true;
      rock.isSpawning_ = false;
      rock.isDropped_ = true; // 飛び散り中

    } else {
      // spawnCount を超えた分は「消えるパーティクル用」にする場合はここで Kill
      // してもOK 今は単純にフィールドにも残さないなら isAlive_ を false
      // にしておく
      rock.isAlive_ = false;
    }
  }
}

void RockManager::Draw(IrufemiEngine * /*engine*/, Camera * /*camera*/) {
  if (!rockRegion_) {
    return;
  }

  // 今フレームのインスタンス情報をクリアする
  rockRegion_->ClearInstances();

  for (const auto &rock : rocks_) {

    // 生存しておらず、プレイヤーにくっついていない岩は描画しない
    if (!rock.isAlive_ && !rock.isAttached_) {
      continue;
    }

    // Transformを生成する
    Transform t{};
    float s = rock.radius_ * 2.0f;
    t.scale = {s, s, s};
    t.rotate = rock.rotate_;
    t.translate = rock.position_;

    rockRegion_->AddInstance(t);
  }

  // Region のインスタンシング描画
  rockRegion_->Draw();
}
