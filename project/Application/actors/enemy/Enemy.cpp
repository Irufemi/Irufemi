#include "Enemy.h"
#include "contents/wall/Wall.h"
#include "function/Random.h"
#include "../healer/HealerActor.h"
#include <limits>
#include <cfloat>

#include "camera/Camera.h"

#include "3D/ObjClass.h"

#include "function/Math.h"
#include "function/Collision.h"

Enemy::Enemy() {}

Enemy::~Enemy() {}

void Enemy::Initialize(Camera* camera, Vector3 pos) {
    camera_ = camera;
    model_ = std::make_unique<ObjClass>();
    model_->Initialize(camera, "cube.obj");
    transform_.translate = pos;
    preferHealer_ = false;
    preferHealerTimer_ = 0;
    targetWall_ = nullptr;
    targetHealer_ = nullptr;
    lastTouchedHealer_ = nullptr;
}

void Enemy::Update(const std::list<Wall*>& walls, const std::list<HealerActor*>& healers) {
    // 毎フレーム、衝突状態をリセット
    isCollidingWithWall_ = false;

    if (!alive_) {
        // カウントダウンでリスポーン判定
        if (respawnCounter_ > 0) {
            --respawnCounter_;
        } else {
            // リスポーン
            alive_ = true;
            // ランダム位置に再配置（適宜範囲は調整）
            float x = Random::GeneratorFloat(-10.0f, 10.0f);
            float y = Random::GeneratorFloat(-10.0f, 10.0f);
            transform_.translate = Vector3{ x, y, 0.0f };
            // 初期速度をリセット
            speed = 0.0f;
            // ターゲットをリセット
            targetWall_ = nullptr;
            targetHealer_ = nullptr;
            lastTouchedHealer_ = nullptr;
        }
        // UpdateAABB と transform 更新はリスポーン後も行う
        UpdateOBB();
        return;
    }

    // ターゲットがまだ無い場合のみ確率をロールして行動を決定する
    if (targetWall_ == nullptr && targetHealer_ == nullptr) {
        if (preferHealerTimer_ <= 0) {
            // roll once and cache result for kPreferHealerFrames
            float prob = Random::GeneratorFloat(0.0f, 1.0f);
            preferHealer_ = (prob <= 0.5f);
            preferHealerTimer_ = kPreferHealerFrames;

            if (preferHealer_) {
                // Healer を狙うモード -> その時点で最も近い修復中の Healer をターゲットにする
                const HealerActor* best = nullptr;
                float bestDist = FLT_MAX;
                for (const HealerActor* ha : healers) {
                    if (!ha) continue;
                    if (!ha->IsAssigned()) continue; // 修復中のもののみ
                    if (ha == lastTouchedHealer_) continue; // 直前に触れたHealerは除外
                    float d = Math::Length(ha->GetPosition() - transform_.translate);
                    if (d < bestDist) { bestDist = d; best = ha; }
                }
                if (best) {
                    targetHealer_ = best;
                } else {
                    // Healer がいなければ近い壁をターゲットにする
                    Wall* nearestWall = nullptr;
                    float nearestDistance = FLT_MAX;
                    for (Wall* wall : walls) {
                        if (!wall) continue;
                        float d = Math::Length(wall->GetPosition() - transform_.translate);
                        if (d < nearestDistance) { nearestDistance = d; nearestWall = wall; }
                    }
                    targetWall_ = nearestWall;
                }
            } else {
                // 壁を壊すモード -> その時点で最も近い Wall をターゲットにする
                Wall* bestW = nullptr;
                float bestDistW = FLT_MAX;
                for (Wall* w : walls) {
                    if (!w) continue;
                    float d = Math::Length(w->GetPosition() - transform_.translate);
                    if (d < bestDistW) { bestDistW = d; bestW = w; }
                }
                targetWall_ = bestW;
            }
        } else {
            --preferHealerTimer_;
        }
    } else {
        // 既にターゲットがある場合はタイマーだけ減らす（再ロールはターゲットが消えるまで行わない）
        if (preferHealerTimer_ > 0) --preferHealerTimer_;
    }

    // 保持しているターゲットがまだ存在するか検証する。存在しなければクリアして次フレームに再ロール可能にする
    if (targetHealer_) {
        bool valid = false;
        for (const HealerActor* ha : healers) {
            if (ha == targetHealer_) { valid = true; break; }
        }
        if (!valid || !targetHealer_->IsAssigned()) {
            targetHealer_ = nullptr;
        }
    }
    if (targetWall_) {
        bool valid = false;
        for (Wall* w : walls) {
            if (w == targetWall_) { valid = true; break; }
        }
        if (!valid) {
            targetWall_ = nullptr;
        }
    }

    // ターゲットに向かって移動
    Vector3 targetPos{ 0.0f, 0.0f, 0.0f };
    bool hasTarget = false;
    if (targetHealer_) {
        hasTarget = true;
        targetPos = targetHealer_->GetPosition();
    } else if (targetWall_) {
        hasTarget = true;
        targetPos = targetWall_->GetPosition();
    }

    if (hasTarget && !isCollidingWithWall_) {
        Vector3 currentPos = transform_.translate;
        Vector3 toTarget = targetPos - currentPos;
        Vector3 desiredDir = Math::Normalize(toTarget);

        // --- 1. 前方の障害物検知 (避けるべき壁を探す) ---
        float lookAheadDistance = 3.0f; // どれくらい先まで見るか
        Vector3 probePos = currentPos + desiredDir * lookAheadDistance;

        Wall* obstacle = nullptr;
        float minObstacleDist = lookAheadDistance;

        for (Wall* wall : walls) {
            if (!wall || wall == targetWall_) continue; // 目的地そのものは無視

            // 簡易的な距離判定（より精密にするならOBBとの交差判定を使用）
            float dist = Math::Length(wall->GetPosition() - currentPos);
            if (dist < lookAheadDistance + 1.0f) { // 近くの壁だけチェック
                // 進行方向と壁の近さを判定
                Vector3 toWall = wall->GetPosition() - currentPos;
                float projection = Math::Dot(toWall, desiredDir);

                if (projection > 0) { // 前方にある
                    Vector3 closestPointOnPath = currentPos + desiredDir * projection;
                    float distFromPath = Math::Length(wall->GetPosition() - closestPointOnPath);

                    if (distFromPath < 2.5f) { // 壁が進行路を塞いでいる（2.5fは壁の幅に合わせて調整）
                        if (projection < minObstacleDist) {
                            minObstacleDist = projection;
                            obstacle = wall;
                        }
                    }
                }
            }
        }

        // --- 2. 回避方向の計算 ---
        Vector3 steeringForce = { 0,0,0 };
        if (obstacle) {
            // 壁の中心から現在の進行路をずらす力を計算
            Vector3 wallPos = obstacle->GetPosition();
            Vector3 toWall = wallPos - currentPos;

            // 進行方向に対して垂直な「逃げ」方向を作る
            // (x, y, 0) の場合、(-y, x, 0) または (y, -x, 0)
            Vector3 leftAvoid = { -desiredDir.y, desiredDir.x, 0.0f };
            Vector3 rightAvoid = { desiredDir.y, -desiredDir.x, 0.0f };

            // どちらがより空いているか（または壁から遠ざかるか）で判断
            if (Math::Dot(leftAvoid, toWall) < Math::Dot(rightAvoid, toWall)) {
                steeringForce = leftAvoid;
            } else {
                steeringForce = rightAvoid;
            }

            // 壁に近ければ近いほど強い力で避ける
            float forceMultiplier = (lookAheadDistance - minObstacleDist) / lookAheadDistance;
            steeringForce = steeringForce * forceMultiplier * 2.0f;
        }

        // --- 3. 最終的な移動方向の合成 ---
        Vector3 finalDir = Math::Normalize(desiredDir + steeringForce);

        speed = 0.1f;
        transform_.translate += finalDir * speed;
    }

    // Healer に接触したか簡易判定
    // 接触を検出したら targetHealer_ をクリアして lastTouchedHealer_ に記憶
    if (targetHealer_) {
        float touchDist = 1.0f; // 接触許容距離(調整可)
        float d = Math::Length(targetHealer_->GetPosition() - transform_.translate);
        if (d <= touchDist) {
            // 接触とみなす
            targetHealer_ = nullptr;
            lastTouchedHealer_ = targetHealer_;
            // また preferHealerTimer_ をリセットして次フレームで再抽選可能にする
            preferHealerTimer_ = 0;
        }
    }

    UpdateOBB();

}

void Enemy::Draw() {

    // 描画物の更新
    model_->SetTransform(transform_);
    model_->Update();

    // 描画物の描画
    if (alive_ && model_) model_->Draw();
}

void Enemy::UpdateOBB()
{
    obb_.center = transform_.translate;
    obb_.size = { width_ / 2.0f, height_ / 2.0f, depth_ / 2.0f };
    Matrix4x4 rotateMatrix = Math::MakeRotateXYZMatrix(transform_.rotate.x, transform_.rotate.y, transform_.rotate.z);
    obb_.orientations[0] = { rotateMatrix.m[0][0], rotateMatrix.m[0][1], rotateMatrix.m[0][2] };
    obb_.orientations[1] = { rotateMatrix.m[1][0], rotateMatrix.m[1][1], rotateMatrix.m[1][2] };
    obb_.orientations[2] = { rotateMatrix.m[2][0], rotateMatrix.m[2][1], rotateMatrix.m[2][2] };
}

const OBB& Enemy::GetOBB() const { return obb_; }

void Enemy::HandleCollision() { speed = 0.0f; }

void Enemy::OnCollisionWithWall(const Wall* wall) {
    const OBB& wallOBB = wall->GetOBB();
    Vector3 pushBackVector = { 0.0f, 0.0f, 0.0f };
    float minOverlap = FLT_MAX;

    // OBBの分離軸テストを再度行い、最小の押し出しベクトルを見つける
    // 15本の分離軸をテスト
    const Vector3* axes[15];
    axes[0] = &obb_.orientations[0];
    axes[1] = &obb_.orientations[1];
    axes[2] = &obb_.orientations[2];
    axes[3] = &wallOBB.orientations[0];
    axes[4] = &wallOBB.orientations[1];
    axes[5] = &wallOBB.orientations[2];
    Vector3 crossProduct;
    crossProduct = Math::Cross(obb_.orientations[0], wallOBB.orientations[0]); axes[6] = &crossProduct;
    crossProduct = Math::Cross(obb_.orientations[0], wallOBB.orientations[1]); axes[7] = &crossProduct;
    crossProduct = Math::Cross(obb_.orientations[0], wallOBB.orientations[2]); axes[8] = &crossProduct;
    crossProduct = Math::Cross(obb_.orientations[1], wallOBB.orientations[0]); axes[9] = &crossProduct;
    crossProduct = Math::Cross(obb_.orientations[1], wallOBB.orientations[1]); axes[10] = &crossProduct;
    crossProduct = Math::Cross(obb_.orientations[1], wallOBB.orientations[2]); axes[11] = &crossProduct;
    crossProduct = Math::Cross(obb_.orientations[2], wallOBB.orientations[0]); axes[12] = &crossProduct;
    crossProduct = Math::Cross(obb_.orientations[2], wallOBB.orientations[1]); axes[13] = &crossProduct;
    crossProduct = Math::Cross(obb_.orientations[2], wallOBB.orientations[2]); axes[14] = &crossProduct;

    for (int i = 0; i < 15; ++i) {
        const Vector3& axis = *axes[i];
        if (Math::Length(axis) < 0.0001f) continue; // 軸がゼロベクトルの場合はスキップ

        Vector3 normalizedAxis = Math::Normalize(axis);

        float projEnemy =
            obb_.size.x * std::abs(Math::Dot(normalizedAxis, obb_.orientations[0])) +
            obb_.size.y * std::abs(Math::Dot(normalizedAxis, obb_.orientations[1])) +
            obb_.size.z * std::abs(Math::Dot(normalizedAxis, obb_.orientations[2]));

        float projWall =
            wallOBB.size.x * std::abs(Math::Dot(normalizedAxis, wallOBB.orientations[0])) +
            wallOBB.size.y * std::abs(Math::Dot(normalizedAxis, wallOBB.orientations[1])) +
            wallOBB.size.z * std::abs(Math::Dot(normalizedAxis, wallOBB.orientations[2]));

        Vector3 distanceVec = obb_.center - wallOBB.center;
        float distance = std::abs(Math::Dot(distanceVec, normalizedAxis));

        float overlap = projEnemy + projWall - distance;
        if (overlap < 0) {
            // 分離軸が見つかったので、この関数が呼ばれるのはおかしいが、安全のためリターン
            return;
        }

        if (overlap < minOverlap) {
            minOverlap = overlap;
            // 押し出し方向を決定
            Vector3 direction = obb_.center - wallOBB.center;
            if (Math::Dot(direction, normalizedAxis) < 0) {
                pushBackVector = -normalizedAxis;
            } else {
                pushBackVector = normalizedAxis;
            }
        }
    }

    // 最小の重なり量で押し出す
    if (minOverlap < FLT_MAX) {
        transform_.translate += pushBackVector * minOverlap;
        UpdateOBB(); // 位置を更新したのでOBBも更新
    }
}

void Enemy::Kill() { alive_ = false; respawnCounter_ = kRespawnFrames; }