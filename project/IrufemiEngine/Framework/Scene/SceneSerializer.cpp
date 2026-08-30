#include "Framework/Scene/SceneSerializer.h"
#include "Framework/Scene/IScene.h"
#include "Framework/Scene/BaseScene.h"
#include "Core/System/IrufemiEngine.h"
#include "Framework/GameObject/GameObject.h"
#include "Core/Utility/Log.h"
#include "Core/Utility/FileSystem.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

std::unordered_map<std::string, nlohmann::json> SceneSerializer::prefabCache_;
std::unordered_map<std::string, std::shared_ptr<GameObject>> SceneSerializer::prefabInstanceCache_;

bool SceneSerializer::Save(IScene* scene, const std::string& sceneName) {
    if (!scene) {
        return false;
    }

    // シーン自身にシリアライズを委譲する
    nlohmann::json root = scene->Serialize();

    std::string pathStr = GetSceneFilePath(scene, sceneName);
    fs::path path(pathStr);

    // ディレクトリが存在しない場合は作成する
    fs::path dir = path.parent_path();
    if (!fs::exists(dir)) {
        fs::create_directories(dir);
    }

    std::ofstream file(path);
    if (file.is_open()) {
        file << root.dump(2); // 4から2に減らして軽量化
        file.close();
        return true;
    }

    return false;
}

bool SceneSerializer::Load(IScene* scene, const std::string& sceneName) {
    if (!scene) {
        return false;
    }

    std::string pathStr = GetSceneFilePath(scene, sceneName);
    fs::path path(pathStr);
    if (!fs::exists(path)) {
        Log::OutPutLog(std::cerr, "[SceneSerializer] Error: File not found: " + pathStr + "\n");
        return false;
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        Log::OutPutLog(std::cerr, "[SceneSerializer] Error: Failed to open file: " + pathStr + "\n");
        return false;
    }

    nlohmann::json root;
    try {
        file >> root;
    } catch (const nlohmann::json::parse_error& e) {
        Log::OutPutLog(std::cerr, "JSON Parse Error in " + pathStr + ": " + std::string(e.what()) + "\n");
        return false;
    }
    file.close();

    // デシリアライズをシーンに委譲
    scene->Deserialize(root);

    return true;
}

bool SceneSerializer::Exists(IScene* scene, const std::string& sceneName) {
    return fs::exists(GetSceneFilePath(scene, sceneName));
}

bool SceneSerializer::SavePrefab(std::shared_ptr<GameObject> obj, const std::string& filepath) {
    if (!obj) {
        return false;
    }

    // 単一のオブジェクトをシリアライズ
    nlohmann::json root = obj->Serialize();

    fs::path path(filepath);
    fs::path dir = path.parent_path();
    if (!dir.empty() && !fs::exists(dir)) {
        fs::create_directories(dir);
    }

    std::ofstream file(path);
    if (file.is_open()) {
        file << root.dump(2);
        file.close();
        return true;
    }

    return false;
}

nlohmann::json SceneSerializer::GetPrefabJson(const std::string& filepath) {
    auto it = prefabCache_.find(filepath);
    if (it != prefabCache_.end()) {
        return it->second;
    }

    fs::path path(filepath);
    if (!fs::exists(path)) {
        return nlohmann::json::object();
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        return nlohmann::json::object();
    }

    nlohmann::json root;
    try {
        file >> root;
    } catch (const nlohmann::json::parse_error& e) {
        Log::OutPutLog(std::cerr, "JSON Parse Error in Prefab " + filepath + ": " + std::string(e.what()) + "\n");
        return nlohmann::json::object();
    }
    file.close();

    prefabCache_[filepath] = root;
    return root;
}

std::shared_ptr<GameObject> SceneSerializer::LoadPrefab(const std::string& filepath) {
    auto it = prefabInstanceCache_.find(filepath);
    std::shared_ptr<GameObject> templateObj;

    if (it != prefabInstanceCache_.end()) {
        templateObj = it->second;
    } else {
        nlohmann::json root = GetPrefabJson(filepath);
        if (root.empty()) {
            return nullptr;
        }

        templateObj = std::make_shared<GameObject>();
        templateObj->Deserialize(root);
        prefabInstanceCache_[filepath] = templateObj;
    }

    // テンプレートからディープコピー (超高速クローン)
    auto obj = templateObj->Clone();
    obj->Initialize();
    return obj;
}

void SceneSerializer::ClearCache() {
    prefabCache_.clear();
    prefabInstanceCache_.clear();
}

std::string SceneSerializer::GetSceneFilePath(IScene* scene, const std::string& sceneName) {
    fs::path dir = FileSystem::GetResourcePath("scenes");
    if (scene && scene->GetEngine()) {
        dir = scene->GetEngine()->GetSceneDirectory();
    }
    fs::path filePath = dir / (sceneName + ".json");
    // Windows環境でもスラッシュ区切りにするための工夫（必要に応じて）
    std::string pathStr = filePath.string();
    std::replace(pathStr.begin(), pathStr.end(), '\\', '/');
    return pathStr;
}
