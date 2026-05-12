#pragma once
#include <vector>
#include <memory>
#include <string>
#include <nlohmann/json.hpp>
#include "Renderer/LineInstanced/LineClass.h"

class ColliderComponent;

/**
 * @class CollisionManager
 * @brief シーン内のすべての当たり判定を管理し、衝突判定を処理するマネージャ
 */
class CollisionManager {
public:
    static CollisionManager& GetInstance() {
        static CollisionManager instance;
        return instance;
    }

    /// @brief 初期化
    void Initialize();

    /// @brief 登録されたコライダーを全てクリアする（シーン切り替え時などに呼ぶ）
    void Clear();
    
    /// @brief コライダーを登録する
    void RegisterCollider(ColliderComponent* collider);
    
    /// @brief コライダーの登録を解除する
    void UnregisterCollider(ColliderComponent* collider);

    /// @brief 毎フレーム呼ばれ、登録された全ペアの判定を行う
    void CheckAllCollisions();

    /// @brief 全コライダーのデバッグ線を描画する
    void DrawDebug();

    /// @brief デバッグ描画フラグのポインタを取得する（ImGui用）
    bool* GetIsDrawDebugLinePtr() { return &isDrawDebugLine_; }

    // --- 動的レイヤー管理 ---
    void LoadLayers(const std::string& filepath);
    void SaveLayers(const std::string& filepath);
    std::vector<std::string>& GetLayerNames() { return layerNames_; }
    void AddLayer(const std::string& name);
    void RemoveLayer(int index);
    
    // エディタ用UI
    void DrawLayerInspectorGUI(uint32_t& layer, uint32_t& mask);

private:
    CollisionManager() = default;
    ~CollisionManager();
    CollisionManager(const CollisionManager&) = delete;
    CollisionManager& operator=(const CollisionManager&) = delete;

    std::vector<ColliderComponent*> colliders_;
    std::unique_ptr<Line3DRegion> debugLine_;
    
    // レイヤー名（最大32個）
    std::vector<std::string> layerNames_;
    std::string layerConfigFilePath_ = "resources/config/layers.json";

    bool isDrawDebugLine_ = true;
};
