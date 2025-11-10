#define NOMINMAX
#include "EnemyManager.h"
#include "math/Vector2.h"
#include "camera/Camera.h"
#include "function/Math.h"
#include <algorithm>
#include <cmath>

#include "ScreenSpace.h"

namespace {

    // 行ベクトル系用：任意軸(単位ベクトル)まわりの軸回転行列
    static Matrix4x4 MakeRotateAxisAngleRow(const Vector3& axisUnit, float angle) {
        Vector3 a = Math::Normalize(axisUnit);
        float c = std::cos(angle), s = std::sin(angle), t = 1.0f - c;

        // まず列ベクトル想定のRcolを作る
        Matrix4x4 m{};
        m.m[0][0] = t*a.x*a.x + c;     m.m[0][1] = t*a.x*a.y - s*a.z; m.m[0][2] = t*a.x*a.z + s*a.y; m.m[0][3] = 0.0f;
        m.m[1][0] = t*a.x*a.y + s*a.z; m.m[1][1] = t*a.y*a.y + c;     m.m[1][2] = t*a.y*a.z - s*a.x; m.m[1][3] = 0.0f;
        m.m[2][0] = t*a.x*a.z - s*a.y; m.m[2][1] = t*a.y*a.z + s*a.x; m.m[2][2] = t*a.z*a.z + c;     m.m[2][3] = 0.0f;
        m.m[3][0] = 0.0f;               m.m[3][1] = 0.0f;               m.m[3][2] = 0.0f;               m.m[3][3] = 1.0f;

        // 行ベクトル系なので転置して返す
        return Math::Transpose(m);
    }
} // namespace

void EnemyManager::Initialize(Camera* camera) {
    tetra_ = std::make_unique<TetraRegion>();
    // 必要ならモデルのエッジ長をここで設定: tetra_->SetEdge(1.0f);
    tetra_->Initialize(camera, "resources/whiteTexture.png");
    tetra_->SetColor(Vector4{ 0.7f,0.4f,0.4f,1.0f });
}

void EnemyManager::Spawn (float pos, Vector2 goal) {
    enemy_.Initialize (pos, goal);
    enemies_.push_back (enemy_);
}

void EnemyManager::EraseEnemy () {
    enemies_.erase (
        std::remove_if (enemies_.begin (), enemies_.end (),
        [](Enemy& enemy) {
            return !enemy.GetIsAlive ();
        }),
        enemies_.end ()
    );
}

void EnemyManager::Draw(Camera* camera) {
    if (!tetra_) return;
    tetra_->ClearInstances();

    const float targetZ = 0.0f;

    // ドリル角度の積算は Update(deltaTime) 側で行う（ここでは積算しない）

    for (auto& e : enemies_) {
        if (!e.GetIsAlive()) continue;

        Vector2 screenPos = e.GetPosition();
        float   pixelR    = e.GetRadius().x;

        Vector3 worldCenter       = ScreenToWorldOnZ(camera, screenPos, targetZ);
        float   worldVertexRadius = ScreenRadiusToWorld(camera, screenPos, pixelR, targetZ);

        // 進行方向（velocity優先、無ければdiffの正規化）
        Vector2 dir2 = e.GetVelocity();
        if (Math::Length(dir2) < 1e-6f) {
            dir2 = Math::Normalize(e.GetDiff());
        }

        // スケールは頂点半径から求める
        const float s = tetra_->ComputeScaleFromVertexRadius(worldVertexRadius);
        const Matrix4x4 S = Math::MakeScaleMatrix(Vector3{ s, s, s });

        // ドリル回転：ローカル先端軸 (1,1,1) 周り（drillAngle_ は Update で更新される）
        const Matrix4x4 Rspin = MakeRotateAxisAngleRow(Vector3{1.0f, 1.0f, 1.0f}, drillAngle_);

        Matrix4x4 Rface = Math::MakeRotateZMatrix(0.0f); // 既定値（無方向時）

        if (Math::Length(dir2) >= 1e-6f) {
            // スクリーンで1px進めた点をZ=0へUnprojectしてワールド方向を得る
            Vector2 dirPixN = Math::Normalize(dir2);
            Vector2 screenNext{ screenPos.x + dirPixN.x, screenPos.y + dirPixN.y };
            Vector3 worldNext = ScreenToWorldOnZ(camera, screenNext, targetZ);

            Vector2 worldDir{ worldNext.x - worldCenter.x, worldNext.y - worldCenter.y };
            if (Math::Length(worldDir) >= 1e-6f) {
                Vector2 dirNorm = Math::Normalize(worldDir);

                // ローカル上頂点のXY方向（(k,k)の方向）
                const Vector2 localTip = Vector2{ 1.0f, 1.0f };
                const float   localTipLen = std::sqrt(localTip.x * localTip.x + localTip.y * localTip.y);
                Vector2       localTipN{ localTip.x / localTipLen, localTip.y / localTipLen };

                // 目標角と基準角の差から回転角を求める
                float angleTarget = std::atan2(dirNorm.y, dirNorm.x);       // ワールドXYでの進行角
                float angleLocal  = std::atan2(localTipN.y, localTipN.x);   // 45°
                float rotateZ     = angleTarget - angleLocal;

                // 回転後のローカル先端が逆向きなら180度足す
                float c = std::cos(rotateZ), s2 = std::sin(rotateZ);
                Vector2 rotatedTip{ c * localTipN.x - s2 * localTipN.y, s2 * localTipN.x + c * localTipN.y };
                float dot = rotatedTip.x * dirNorm.x + rotatedTip.y * dirNorm.y;
                if (dot < 0.0f) { rotateZ += 3.14159265358979323846f; }

                // 正規化（-PI..PI）
                if (rotateZ > 3.14159265358979323846f) rotateZ -= 2.0f * 3.14159265358979323846f;
                if (rotateZ < -3.14159265358979323846f) rotateZ += 2.0f * 3.14159265358979323846f;

                Rface = Math::MakeRotateZMatrix(rotateZ);
            }
        }

        const Matrix4x4 T = Math::MakeTranslateMatrix(worldCenter);

        // 行ベクトル系: v * (S · Rspin(ローカル先端軸) · Rface · T)
        const Matrix4x4 world = Math::Multiply(S, Math::Multiply(Rspin, Math::Multiply(Rface, T)));

        tetra_->AddInstanceWorld(world);
    }

    tetra_->Draw();
}

// --- フレームレート非依存更新 ---
void EnemyManager::Update(float deltaTime) {
    // deltaTime は秒単位（例: 1/60.0f）
    drillAngle_ += drillSpeedRadPerSec_ * deltaTime;
    const float twoPi = 6.283185307179586f;
    drillAngle_ = std::fmod(drillAngle_, twoPi);
    if (drillAngle_ < 0.0f) drillAngle_ += twoPi;
}

// --- 速度設定ヘルパ（互換と推奨rad/sec両方用意） ---
void EnemyManager::SetDrillSpeedRadPerFrame(float radPerFrame) {
    // 互換: rad/frame -> rad/sec（想定FPS = 60）
    const float assumedFps = 60.0f;
    drillSpeedRadPerSec_ = radPerFrame * assumedFps;
}

void EnemyManager::SetDrillSpeedRadPerSec(float radPerSec) {
    drillSpeedRadPerSec_ = radPerSec;
}

void EnemyManager::SetDrillRPM(float rpm, float /*assumedFps*/) {
    // RPM -> rad/sec
    const float twoPi = 6.283185307179586f;
    float radPerSec = rpm * twoPi / 60.0f;
    drillSpeedRadPerSec_ = radPerSec;
}