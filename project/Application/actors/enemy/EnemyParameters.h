#pragma once
#include <string>
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"

class EnemyParameters {
public:
  // シングルトンインスタンスの取得
  static EnemyParameters* GetInstance();

  // JSONファイルからの読み込み
  void Load(const std::string& filepath);

  // パラメータの取得
  int GetBodyHP() const { return bodyHP_; }
  int GetHeadLeftHP() const { return headLeftHP_; }
  int GetHeadMidHP() const { return headMidHP_; }
  int GetHeadRightHP() const { return headRightHP_; }

  float GetBlowSpeed() const { return blowSpeed_; }
  void SetBlowSpeed(float speed) { blowSpeed_ = speed; }

  float GetDisappearTime() const { return disappearTime_; }
  void SetDisappearTime(float time) { disappearTime_ = time; }

  const Vector3& GetBodyOBBSize() const { return bodyOBBSize_; }
  void SetBodyOBBSize(const Vector3& size) { bodyOBBSize_ = size; }

  const Vector3& GetHeadOBBSize() const { return headOBBSize_; }
  void SetHeadOBBSize(const Vector3& size) { headOBBSize_ = size; }

  float GetDamageFlashDuration() const { return damageFlashDuration_; }
  void SetDamageFlashDuration(float duration) { damageFlashDuration_ = duration; }

  const Vector4& GetDamageFlashColor() const { return damageFlashColor_; }
  void SetDamageFlashColor(const Vector4& color) { damageFlashColor_ = color; }

private:
  EnemyParameters() = default;
  ~EnemyParameters() = default;
  EnemyParameters(const EnemyParameters&) = delete;
  EnemyParameters& operator=(const EnemyParameters&) = delete;

  int bodyHP_ = 100;
  int headLeftHP_ = 50;
  int headMidHP_ = 150;
  int headRightHP_ = 50;

  float blowSpeed_ = 0.5f;
  float disappearTime_ = 3.0f;

  /**
   * @brief Body部位のOBB（当たり判定）のハーフサイズ
   * @details 円柱形状（半径2.85、高さ2.0）を近似するため、
   * X, Z には半径の 2.85 を、Y には高さの半分の 1.0 を設定。
   */
  Vector3 bodyOBBSize_ = { 2.85f, 1.0f, 2.85f };
  Vector3 headOBBSize_ = { 1.5f, 1.0f, 1.5f };

  float damageFlashDuration_ = 0.2f;
  Vector4 damageFlashColor_ = { 1.0f, 0.6f, 0.6f, 1.0f };
};
