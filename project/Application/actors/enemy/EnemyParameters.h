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

  Vector3 bodyOBBSize_ = { 1.5f, 1.0f, 1.5f };
  Vector3 headOBBSize_ = { 1.5f, 1.0f, 1.5f };

  float damageFlashDuration_ = 0.2f;
  Vector4 damageFlashColor_ = { 1.0f, 0.6f, 0.6f, 1.0f };
};
