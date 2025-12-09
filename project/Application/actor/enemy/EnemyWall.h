#pragma once
#include "3D/ObjClass.h"
#include "camera/Camera.h"
#include "math/Vector3.h"
#include <memory>
#include <vector>

struct EnemyWall {
  Vector3 position; // ワールド座標（中心）
  float halfSizeX;  // X方向半径（3.0 の半分 → 1.5）
  float halfSizeZ;  // Z方向半径（1.0 の半分 → 0.5）
  float lifeTime;   // 残り寿命（秒）
  bool active;      // 使用中フラグ

  // 落下アニメーション用
  float groundY;         // 着地する高さ（地面）
  float fallStartHeight; // groundY からどれだけ上から落ちてくるか
  float fallDuration;    // 落下にかかる時間
  float fallTimer;       // 残り落下時間
  float warningTime;     // 地面に予告だけ出している時間
  bool hasLanded;        // 落ち切って着地済みかどうか

  EnemyWall()
      : position(0.0f, 0.0f, 0.0f), halfSizeX(1.5f), halfSizeZ(1.5f),
        lifeTime(0.0f), active(false), groundY(0.0f), fallStartHeight(8.0f),
        fallDuration(0.5f), fallTimer(0.0f), warningTime(0.0f),
        hasLanded(false) {}
};

class EnemyWallManager {
public:
  EnemyWallManager();

  // ステージ中心と半径をセット + 描画用カメラも渡す
  void Initialize(Camera *camera, const Vector3 &stageCenter,
                  float stageRadius);

  // 毎フレーム更新
  void Update(float deltaTime);

  // 描画
  void Draw();

  // 壁生成アクション：敵の位置を受け取って「最大3つ」生成を試みる
  void SpawnWalls(const Vector3 &enemyPos, const Vector3 &playerPos);

  // 外から当たり判定に使えるように参照を返す
  const std::vector<EnemyWall> &GetWalls() const { return walls_; }

  // 円（プレイヤー・弾など）と壁の当たり判定
  // 当たっている壁のインデックスを返す（なければ -1）
  int CheckCollisionCircle(const Vector3 &center, float radius) const;

  // プレイヤーが壁にぶつかったときに呼ぶ想定（岩半減処理は別側で）
  void OnPlayerHitWall(int wallIndex);

  // 敵が壁にぶつかったときに呼ぶ（仕様：敵に当たったら壁消滅）
  void OnEnemyHitWall(int wallIndex);

  // 設定値
  void SetWallLifeTime(float lifeTime) { wallLifeTime_ = lifeTime; }
  void SetMaxWallCount(int maxCount) { maxWallCount_ = maxCount; }

  // 壁の落下・予告設定
  void SetWallFallParameters(float startHeight, float duration,
                             float warningTime) {
    wallFallStartHeight_ = startHeight;
    wallFallDuration_ = duration;
    wallWarningTime_ = warningTime;
  }

private:
  // カメラ
  Camera *camera_ = nullptr;

  // 壁本体モデル
  std::vector<std::unique_ptr<ObjClass>> models_;

  // 位置予測用のマーカー（赤い円など）のモデル
  std::vector<std::unique_ptr<ObjClass>> warningModels_;

  Vector3 stageCenter_;
  float stageRadius_{};

  std::vector<EnemyWall> walls_;

  float wallLifeTime_ = 8.0f; // 壁の寿命（秒）とりあえず8秒ぐらい
  int maxWallCount_ = 9;      // 同時最大生成数

  // 上から落ちてくるためのパラメータ
  float wallFallStartHeight_ =
      12.0f;                       // 何メートル上から落ちてくるか（前より高く）
  float wallFallDuration_ = 0.4f; // 落下時間
  float wallWarningTime_ = 0.6f;  // 着弾予告だけ出している時間

  // 影（予測マーカー）のスケール範囲
  float warningScaleMin_ = 0.4f; // 落下開始前〜開始直後の最小スケール
  float warningScaleMax_ = 1.6f; // 着地直前・直後の最大スケール

  // 内部用：この位置に壁を置いて良いかチェック
  bool CanPlaceWallAt(const Vector3 &pos, const Vector3 &enemyPos,
                      const Vector3 &playerPos) const;

  // 内部用：リング領域にランダムな位置を生成
  Vector3 GenerateRandomPosition(const Vector3 &enemyPos) const;
};
