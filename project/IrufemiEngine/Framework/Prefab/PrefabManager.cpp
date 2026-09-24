#include "Framework/Prefab/PrefabManager.h"
#include "Framework/GameObject/GameObject.h"
#include "Core/Utility/JsonUtility.h"

const nlohmann::json& PrefabManager::GetPrefabJson(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = jsonCache_.find(filepath);
    if (it != jsonCache_.end()) {
        return it->second;
    }

    nlohmann::json root;
    if (!Irufemi::JsonUtility::LoadFromFile(filepath, root)) {
        static const nlohmann::json kEmptyJson = nlohmann::json::object();
        return kEmptyJson;
    }

    auto [insertedIt, success] = jsonCache_.emplace(filepath, std::move(root));
    return insertedIt->second;
}

std::shared_ptr<GameObject> PrefabManager::GetTemplate(const std::string& filepath) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = templateCache_.find(filepath);
        if (it != templateCache_.end()) {
            return it->second;
        }
    }

    const nlohmann::json& root = GetPrefabJson(filepath);
    if (root.empty()) {
        return nullptr;
    }

    auto templateObj = std::make_shared<GameObject>();
    templateObj->Deserialize(root);

    std::lock_guard<std::mutex> lock(mutex_);
    // Double-checked locking
    auto it = templateCache_.find(filepath);
    if (it != templateCache_.end()) {
        return it->second;
    }
    templateCache_[filepath] = templateObj;
    return templateObj;
}

std::shared_ptr<GameObject> PrefabManager::Instantiate(const std::string& filepath) {
    auto templateObj = GetTemplate(filepath);
    if (!templateObj) {
        return nullptr;
    }

    // テンプレートからディープコピー (高速クローン)
    auto obj = templateObj->Clone();
    return obj;
}

PrefabManager::~PrefabManager() {
    ClearCache();
}

void PrefabManager::ClearCache() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [path, templateObj] : templateCache_) {
        if (templateObj && !templateObj->IsDestroyed()) {
            templateObj->Destroy();
        }
    }
    jsonCache_.clear();
    templateCache_.clear();
}
