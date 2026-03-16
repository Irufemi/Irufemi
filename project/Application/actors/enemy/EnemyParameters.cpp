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

    if (enemy.contains("blow_speed")) {
        blowSpeed_ = enemy["blow_speed"];
    }
    if (enemy.contains("disappear_time")) {
        disappearTime_ = enemy["disappear_time"];
    }
    if (enemy.contains("damage_flash_duration")) {
        damageFlashDuration_ = enemy["damage_flash_duration"];
    }
    if (enemy.contains("damage_flash_color") && enemy["damage_flash_color"].is_array() && enemy["damage_flash_color"].size() == 4) {
        damageFlashColor_ = { enemy["damage_flash_color"][0], enemy["damage_flash_color"][1], enemy["damage_flash_color"][2], enemy["damage_flash_color"][3] };
    }
    if (enemy.contains("body") && enemy["body"].contains("obb_size") && enemy["body"]["obb_size"].is_array() && enemy["body"]["obb_size"].size() == 3) {
        bodyOBBSize_ = { enemy["body"]["obb_size"][0], enemy["body"]["obb_size"][1], enemy["body"]["obb_size"][2] };
    }
    if (enemy.contains("head_mid") && enemy["head_mid"].contains("obb_size") && enemy["head_mid"]["obb_size"].is_array() && enemy["head_mid"]["obb_size"].size() == 3) {
        headOBBSize_ = { enemy["head_mid"]["obb_size"][0], enemy["head_mid"]["obb_size"][1], enemy["head_mid"]["obb_size"][2] };
    }
  }
}
