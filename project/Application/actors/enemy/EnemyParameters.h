#pragma once
#include <string>

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
};
