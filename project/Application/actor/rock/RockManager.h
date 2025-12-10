#pragma once
#include <memory>
#include <vector>

#include "Rock.h"
#include"audio/Se.h"

class IrufemiEngine;
class Camera;
class Player;
class Region;
class Field;

// Rock 全体を管理するクラス
class RockManager {
public:
  /// <summary>
  /// 初期化
  /// </summary>
  void Initialize(Camera *camera);

  /// <summary>
  /// 指定位置に岩を追加する
  /// </summary>
  void AddRock(const Vector3 &pos, float radius);

  /// <summary>
  /// 自動スポーンを行う範囲を設定する
  /// </summary>
  void SetSpawnArea(const Vector3 &minPos, const Vector3 &maxPos);

  /// <summary>
  /// 更新処理
  ///
  /// 移動更新
  /// スポーン処理
  /// プレイヤーとの当たり判定
  /// </summary>
  void Update(Player *player);

  /// <summary>
  /// 描画処理
  ///
  /// Regionを使用した一括描画
  /// </summary>
  void Draw(IrufemiEngine *engine, Camera *camera);

  /// <summary>
  /// 岩リストへのアクセス
  /// </summary>
  const std::vector<Rock> &GetRocks() const { return rocks_; }
  std::vector<Rock> &GetRocks() { return rocks_; }

  /// <summary>
  /// fieldのセッター
  /// </summary>
  void SetField(Field *field) { field_ = field; }

private:
  // 岩リスト
  std::vector<Rock> rocks_;

  //  インスタンシング描画用
  std::unique_ptr<Region> rockRegion_ = nullptr;

  // 自動スポーン用
  Vector3 spawnMin_{-10.0f, 0.0f, 5.0f};
  Vector3 spawnMax_{10.0f, 0.0f, 15.0f};
  float spawnInterval_ = 2.0f;
  float spawnTimer_ = 0.0f;
  size_t maxAlive_ = 20;

  // フィールドへのポインタ
  Field *field_ = nullptr;

  //SEの初期化
  Se rockAttachedSE_;

  /// <summary>
  /// 岩自身の更新処理
  /// スポーンアニメーションなど
  /// </summary>
  void UpdateRocks(float deltaTime);

  /// <summary>
  /// 一定時間ごとに岩を自動リスポーンさせる
  /// </summary>
  void AutoSpawn(float deltaTime);
  void SpawnRandomRock();

public:
  /// <summary>
  /// 纏っている岩をすべてリセット
  /// </summary>
  void ResetAttachedRocks();

  /// <summary>
  /// 纏っている岩を半分だけ外す
  /// </summary>
  void HalveAttachedRocks(int numToDetach);

  /// <summary>
  /// 纏っている岩の数
  /// </summary>
  /// <returns></returns>
  int GetAttachedCount() {
    int count = 0;
    for (const auto &rock : rocks_) {
      if (rock.isAttached_) {
        ++count;
      }
    }
    return count;
  }

  /// <summary>
  /// 外す岩を決める
  /// </summary>
  /// <param name="lostCount"></param>
  /// <returns></returns>
  std::vector<int> SelectDetachedRocks(int lostCount);

  void SpawnDroppedRocks(const std::vector<int> &detachedList, int spawnCount,
                         const Vector3 &playerPos, const Vector3 &knockbackDir);

private:
  // プレイヤーのポインタ
  Player *lastPlayer_ = nullptr;
};
