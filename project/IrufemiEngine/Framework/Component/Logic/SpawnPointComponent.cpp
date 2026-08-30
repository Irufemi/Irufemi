#include "Framework/Component/Logic/SpawnPointComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Core/Utility/Log.h"

void SpawnPointComponent::OnRegisterProperties() {
    Component::OnRegisterProperties();
    // プロパティとしてエディタに公開（ImGui自動描画やシリアライズ対象になる前提）
    RegisterProperty("Wave ID", &waveId_);
    RegisterProperty("Enemy Type", &enemyType_);
}

void SpawnPointComponent::Initialize() {
    // 依存関係をクリーンに保つため、WaveManager等への直接登録は行わず
    // ゲーム側(WaveManager等)が初期化時にシーンからこのコンポーネントを集めてキャッシュする手法をとる
}

nlohmann::json SpawnPointComponent::Serialize() {
    nlohmann::json j = Component::Serialize();
    j["data"]["waveId"] = waveId_;
    j["data"]["enemyType"] = enemyType_;
    return j;
}

void SpawnPointComponent::Deserialize(const nlohmann::json& json) {
    Component::Deserialize(json);
    if (json.contains("data")) {
        const auto& data = json["data"];
        if (data.contains("waveId")) waveId_ = data["waveId"].get<std::string>();
        if (data.contains("enemyType")) enemyType_ = data["enemyType"].get<std::string>();
    }
}
