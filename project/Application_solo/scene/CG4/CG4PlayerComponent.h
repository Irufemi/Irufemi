#pragma once

#include "Framework/Component/Component.h"
#include <string>

/**
 * @class CG4PlayerComponent
 * @brief CG4シーン専用のプレイヤー操作コンポーネント
 * @details ゲームパッドおよびキーボード入力からキャラクターを移動・旋回させる処理を担当します。
 */
class CG4PlayerComponent : public Component {
public:
    CG4PlayerComponent();
    ~CG4PlayerComponent() override;

    // Componentの基本インターフェース
    void Initialize() override;
    void Update() override;
    
    // シリアライズ対応
    nlohmann::json Serialize() override;
    void Deserialize(const nlohmann::json& j) override;
    
    // エディタへのプロパティ公開
    void OnRegisterProperties() override;
    
    std::string GetComponentName() const override { return "CG4PlayerComponent"; }

private:
    float moveSpeed_ = 5.0f;
    float turnSpeed_ = 10.0f; // 旋回の滑らかさ（Lerp等を使う場合）

    // アニメーション制御用
    std::string currentAnimState_ = "";
    bool isCrouching_ = false;
    float fadeDuration_ = 0.2f;

    std::string idleAnimName_ = "sample/walk.gltf";
    std::string walkAnimName_ = "sample/walk.gltf";
    std::string crouchIdleAnimName_ = "sample/sneakWalk.gltf";
    std::string crouchWalkAnimName_ = "sample/sneakWalk.gltf";
};
