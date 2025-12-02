#include "RockManager.h"

#include "3D/Region.h"
#include "Application/camera/Camera.h"
#include "actor/player/Player.h"
#include "contents/GameFunction.h"
#include "engine/IrufemiEngine.h"
#include "function/Math.h" // MakeAffine など、行列系を使うなら
#include <cmath>
#include <random>

namespace {

/// <summary>
/// 正規化
/// </summary>
/// <param name="v"></param>
/// <returns></returns>
Vector3 NormalizeVec(const Vector3 &v) {
  float lenSq = v.x * v.x + v.y * v.y + v.z * v.z;
  if (lenSq <= 0.0f) {
    return {0.0f, 0.0f, 0.0f};
  }
  float invLen = 1.0f / std::sqrt(lenSq);
  return {v.x * invLen, v.y * invLen, v.z * invLen};
}

} // namespace

void RockManager::Initialize(Camera *camera) {
  // Rock用 Region を生成
  rockRegion_ = std::make_unique<Region>();
  // ここでは block.obj を流用。岩専用モデルがあるならそのファイル名に。
  rockRegion_->Initialize(camera, "block.obj");

  // 必要ならここで初期岩を足してもOK
  // AddRock({0.0f, 0.0f, 5.0f}, 0.5f);
}

void RockManager::AddRock(const Vector3 &pos, float radius) {
  // 上限超えそうなら足さない（保険）
  if (rocks_.size() >= maxAlive_) {
    return;
  }
  rocks_.emplace_back(pos, radius);
}

void RockManager::SetSpawnArea(const Vector3 &minPos, const Vector3 &maxPos) {
  spawnMin_ = minPos;
  spawnMax_ = maxPos;
}

void RockManager::Update(Player *player) {
  const float dt = 1.0f / 60.0f; // 仮フレーム時間

  UpdateRocks(dt);
  AutoSpawn(dt);

  // 2) プレイヤーがいなければ何もしない
  if (!player) {
    return;
  }

  // 3) プレイヤー情報を取得
  const Vector3 &pPos = player->GetPosition();
  const float pRadius = player->GetRadius();

  // 4) すべての岩とプレイヤーの当たり判定
  for (auto &rock : rocks_) {

    if (rock.isAttached_) {
      Vector3 rotated = player->RotateLocalDir(rock.localDir_);
      rock.position_ = pPos + rotated * rock.distanceFromPlayer_;
      continue;
    }

    if (!rock.isAlive_) {
      continue;
    }

    // 球 vs 球の当たり判定
    if (GameFunction::IsHitSphere(pPos, pRadius, rock.position_,
                                  rock.radius_)) {

      // ★ ヒットした：岩を「拾う」扱いにする
      rock.isAttached_ = true;

      // この岩は消す
      rock.Kill();

      // プレイヤーに岩を1つ加算
      //   - Player::AddRock() 内で0未満クランプ済み
      player->AddRock(1);

      // プレイヤーから見た方向＆距離を保存
      Vector3 diff = rock.position_ - pPos;
      Vector3 dir = NormalizeVec(diff);

      // プレイヤーのローカルに反映させる
      rock.localDir_ = player->WorldDirToLocal(dir);

      // プレイヤーとの距離
      rock.distanceFromPlayer_ = pRadius + rock.radius_;

      // 一応、1フレームに複数個拾いたければ continue
      // ひとつだけにしたければ break;
      // ここでは複数拾えるようにしておく
    }
  }
}

void RockManager::UpdateRocks(float deltaTime) {
  for (auto &r : rocks_) {
    if (!r.isAlive_)
      continue;
    r.Update(deltaTime);
  }

  // ★ 死んだ岩を配列から削除
  rocks_.erase(std::remove_if(
                   rocks_.begin(), rocks_.end(),
                   [](const Rock &r) { return !r.isAlive_ && !r.isAttached_; }),
               rocks_.end());
}

void RockManager::AutoSpawn(float deltaTime) {
  spawnTimer_ += deltaTime;

  // 生きてる個数（または配列サイズ）で上限チェック
  size_t alive = 0;
  for (const auto &r : rocks_) {
    if (r.isAlive_) {
      ++alive;
    }
  }
  if (alive >= maxAlive_) {
    return;
  }
  OutputDebugStringA((std::to_string(alive) + "\n").c_str());

  if (spawnTimer_ >= spawnInterval_) {
    spawnTimer_ = 0.0f;
    SpawnRandomRock();
  }
}

void RockManager::SpawnRandomRock() {
  static std::mt19937 rng{std::random_device{}()};

  std::uniform_real_distribution<float> distX(spawnMin_.x, spawnMax_.x);
  std::uniform_real_distribution<float> distZ(spawnMin_.z, spawnMax_.z);

  Vector3 pos{};
  pos.x = distX(rng);
  pos.y = spawnMin_.y; // 高さは固定
  pos.z = distZ(rng);

  float radius = 0.5f;
  AddRock(pos, radius);
}

void RockManager::Draw(IrufemiEngine * /*engine*/, Camera * /*camera*/) {
  if (!rockRegion_) {
    return;
  }

  // いったん全部クリアして、今フレームの岩を積み直す
  rockRegion_->ClearInstances();

  for (const auto &rock : rocks_) {
    if (!rock.isAlive_ && !rock.isAttached_) {
      continue;
    }

    // Rock → Transform 変換
    Transform t{};
    float s = rock.radius_ * 2.0f; // 半径→直径にしてスケール
    t.scale = {s, s, s};
    t.rotate = {0.0f, 0.0f, 0.0f};
    t.translate = rock.position_;

    rockRegion_->AddInstance(t);
  }

  // Region のインスタンシング描画
  rockRegion_->Draw();
}
