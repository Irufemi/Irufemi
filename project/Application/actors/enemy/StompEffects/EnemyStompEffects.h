#pragma once
#include "core/math/Transform.h"
#include <memory>

// 前方宣言
class Camera;
class Player;
class ObjClass;

class EnemyStompEffects {
public:
    // --- パラメータ調整用構造体 ---
    // 全てのマジックナンバーをここに集約しました。
    // ヘッダーの数値を書き換えるだけで挙動を調整できます。
    struct Parameters {
        // 1. 爆発の設定
        float explosionDuration = 2.0f;            // 爆発が消えるまでの時間（秒）
        float explosionMaxRadius = 40.0f;          // 爆発の最大半径
        float explosionInitialAlpha = 0.8f;        // 爆発開始時の透明度
        float explosionDamageActiveTime = 0.3f;    // 爆発のダメージ判定が有効な進捗（0.0～1.0）

        // 2. リング（衝撃波）の設定
        Vector3 ringScale = { 1.2f, 0.5f, 1.2f };  // リングの大きさ
        float ringDuration = 5.0f;                 // リングが消えるまでの時間（秒）
        float ringMaxRadius = 160.0f;              // リングが到達する最大半径
        float ringThickness = 2.5f;                // リングの当たり判定の幅（ドーナツの厚み）
        float ringHeight = 0.01f;                  // リングモデル自体のYスケール（厚み）
        float ringAlpha = 0.5f;                    // リングの透明度（今回は固定）
        float ringGroundOffset = -2.6f;            // ★リングの高さ。足元なら0.0、高いならマイナス値を試してください
    };

    /// @brief 初期化
    void Initialize(Camera* camera);

    /// @brief 更新処理
    void Update(float deltaTime, Player* player);

    /// @brief 描画処理
    void Draw();

    /// @brief エフェクトの開始
    void Fire(const Vector3& position);

    /// @brief エフェクトが実行中かどうか
    bool IsActive() const { return isActive_; }

    /// @brief パラメータを外部から設定する
    void SetParameters(const Parameters& params) { params_ = params; }

private:
    // --- 外部参照 ---
    Camera* camera_ = nullptr;
    std::unique_ptr<ObjClass> explosionObj_ = nullptr;
    std::unique_ptr<ObjClass> ringObj_ = nullptr;

    // --- トランスフォーム ---
    Transform explosionTransform_;
    Transform ringTransform_;

    // --- 状態管理 ---
    bool isActive_ = false;
    float timer_ = 0.0f;
    Vector3 basePosition_; // 発生時の中心座標を保持

    // パラメータの実体
    Parameters params_;

    // 多段ヒット防止フラグ
    bool hasDealtExplosionDamage_ = false;
    bool hasDealtRingDamage_ = false;

    // ヘルパー：線形補間
    float Lerp(float start, float end, float t) {
        return start + (end - start) * t;
    }
};