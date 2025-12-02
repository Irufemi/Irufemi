#include "RockManager.h"

#include "engine/IrufemiEngine.h"
#include "Application/camera/Camera.h"
#include "3D/Region.h"
#include "function/Math.h"     // MakeAffine など、行列系を使うなら
#include <random>

void RockManager::Initialize(Camera* camera) {
    // Rock用 Region を生成
    rockRegion_ = std::make_unique<Region>();
    // ここでは block.obj を流用。岩専用モデルがあるならそのファイル名に。
    rockRegion_->Initialize(camera, "block.obj");

    // 必要ならここで初期岩を足してもOK
    // AddRock({0.0f, 0.0f, 5.0f}, 0.5f);
}

void RockManager::AddRock(const Vector3& pos, float radius) {
    // 上限超えそうなら足さない（保険）
    if (rocks_.size() >= maxAlive_) { return; }
    rocks_.emplace_back(pos, radius);
}

void RockManager::SetSpawnArea(const Vector3& minPos, const Vector3& maxPos) {
    spawnMin_ = minPos;
    spawnMax_ = maxPos;
}

void RockManager::Update(Player* /*player*/) {
    const float dt = 1.0f / 60.0f;  // 仮フレーム時間

    UpdateRocks(dt);
    AutoSpawn(dt);
}

void RockManager::UpdateRocks(float deltaTime) {
    for (auto& r : rocks_) {
        if (!r.isAlive_) continue;
        r.Update(deltaTime);
    }
}

void RockManager::AutoSpawn(float deltaTime) {
    spawnTimer_ += deltaTime;

    // 生きてる個数（または配列サイズ）で上限チェック
    size_t alive = 0;
    for (const auto& r : rocks_) {
        if (r.isAlive_) { ++alive; }
    }
    if (alive >= maxAlive_) {
        return;
    }

    if (spawnTimer_ >= spawnInterval_) {
        spawnTimer_ = 0.0f;
        SpawnRandomRock();
    }
}

void RockManager::SpawnRandomRock() {
    static std::mt19937 rng{ std::random_device{}() };

    std::uniform_real_distribution<float> distX(spawnMin_.x, spawnMax_.x);
    std::uniform_real_distribution<float> distZ(spawnMin_.z, spawnMax_.z);

    Vector3 pos{};
    pos.x = distX(rng);
    pos.y = spawnMin_.y; // 高さは固定
    pos.z = distZ(rng);

    float radius = 0.5f;
    AddRock(pos, radius);
}

void RockManager::Draw(IrufemiEngine* /*engine*/, Camera* /*camera*/) {
    if (!rockRegion_) { return; }

    // いったん全部クリアして、今フレームの岩を積み直す
    rockRegion_->ClearInstances();

    for (const auto& rock : rocks_) {
        if (!rock.isAlive_) { continue; }

        // Rock → Transform 変換
        Transform t{};
        float s = rock.radius_ * 2.0f; // 半径→直径にしてスケール
        t.scale = { s, s, s };
        t.rotate = { 0.0f, 0.0f, 0.0f };
        t.translate = rock.position_;

        rockRegion_->AddInstance(t);
    }

    // Region のインスタンシング描画
    rockRegion_->Draw();
}
