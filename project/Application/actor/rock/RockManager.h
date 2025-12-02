#pragma once
#include <vector>
#include <memory>

#include "Rock.h"

class IrufemiEngine;
class Camera;
class Player;
class Region;        // ★ 追加：Regionを前方宣言
class Field;

// Rock 全体を管理するクラス
class RockManager {
public:
    // ★ カメラを受け取るように変更（RegionのInitializeに必要）
    void Initialize(Camera* camera);

    // 岩を1個追加（ステージ読み込みなどで使う）
    void AddRock(const Vector3& pos, float radius);

    // 自動スポーンエリア設定（任意）
    void SetSpawnArea(const Vector3& minPos, const Vector3& maxPos);

    // Player との当たり判定まで含めた更新
    void Update(Player* player);

    // 描画
    //  ※ engine / camera 引数は互換用に残しておく（中では使わない）
    void Draw(IrufemiEngine* engine, Camera* camera);

    //追加：岩リストへのアクセス
    const std::vector<Rock>& GetRocks() const { return rocks_; }
    std::vector<Rock>& GetRocks() { return rocks_; }

    //追加：Field を渡すためのセッター
    void SetField(Field* field) { field_ = field; }


private:
    std::vector<Rock> rocks_;

    // ★ Rock用 Region（インスタンシング描画用）
    std::unique_ptr<Region> rockRegion_ = nullptr;

    // 自動スポーン用
    Vector3 spawnMin_{ -10.0f, 0.0f, 5.0f };
    Vector3 spawnMax_{ 10.0f, 0.0f, 15.0f };
    float   spawnInterval_ = 2.0f;
    float   spawnTimer_ = 0.0f;
    size_t  maxAlive_ = 20;

    //追加：丸フィールドへのポインタ
    Field* field_ = nullptr;

    void UpdateRocks(float deltaTime);
    void AutoSpawn(float deltaTime);
    void SpawnRandomRock();
};
