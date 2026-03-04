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

private:
  EnemyParameters() = default;
  ~EnemyParameters() = default;
  EnemyParameters(const EnemyParameters&) = delete;
  EnemyParameters& operator=(const EnemyParameters&) = delete;

  int bodyHP_ = 100;
  int headLeftHP_ = 50;
  int headMidHP_ = 150;
  int headRightHP_ = 50;
};
