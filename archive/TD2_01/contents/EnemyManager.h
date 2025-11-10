#pragma once
#include "Enemy.h"
#include <vector>

#include "3D/TetraRegion.h"
#include <memory>

class Camera;

class EnemyManager {
public:
    void Initialize(Camera* camera);
    void Spawn(float pos, Vector2 goal);
    void EraseEnemy();
    void Draw(Camera* camera);

    // フレームレート非依存の更新（秒単位）
    void Update(float deltaTime);

    std::vector<Enemy>& GetEnemies() { return enemies_; }

    // ドリル速度の設定
    void SetDrillSpeedRadPerFrame(float radPerFrame); // 互換: rad/frame -> rad/sec (assume 60fps)
    void SetDrillSpeedRadPerSec(float radPerSec);     // rad/sec (推奨)
    void SetDrillRPM(float rpm, float assumedFps = 60.0f); // RPM -> rad/sec (assumedFps は互換性保持)

private:
    // 描画生成物
    std::unique_ptr<TetraRegion> tetra_ = nullptr;

private:
    // 敵集合
    Enemy enemy_;
    std::vector<Enemy> enemies_;

    // ドリル回転用
    float drillAngle_ = 0.0f;              // 積算角 [rad]
    float drillSpeedRadPerSec_ = 6.0f;     // 回転速度 [rad/sec]（既定: 0.1rad/frame @60fps -> 6rad/sec）
};

