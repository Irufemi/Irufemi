#include "EnemyParameters.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

EnemyParameters* EnemyParameters::GetInstance() {
  static EnemyParameters instance;
  return &instance;
}

void EnemyParameters::Load(const std::string& filepath) {
  std::ifstream file(filepath);
  if (!file.is_open()) {
    return; // ファイルが開けなければデフォルト値のまま
  }

  json j;
  try {
    file >> j;
  } catch (json::parse_error&) {
    return; // パースエラー時もデフォルト値のまま
  }

  if (j.contains("enemy")) {
    const auto& enemy = j["enemy"];
    
    if (enemy.contains("body") && enemy["body"].contains("hp")) {
      bodyHP_ = enemy["body"]["hp"];
    }
    if (enemy.contains("head_left") && enemy["head_left"].contains("hp")) {
      headLeftHP_ = enemy["head_left"]["hp"];
    }
    if (enemy.contains("head_mid") && enemy["head_mid"].contains("hp")) {
      headMidHP_ = enemy["head_mid"]["hp"];
    }
    if (enemy.contains("head_right") && enemy["head_right"].contains("hp")) {
      headRightHP_ = enemy["head_right"]["hp"];
    }
  }
}
