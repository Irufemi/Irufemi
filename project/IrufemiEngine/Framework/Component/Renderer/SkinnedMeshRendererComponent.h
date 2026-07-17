#pragma once

#include "../Component.h"
#include "../../../Renderer/Object/3D/AnimationModel/AnimatedMeshObject.h"
#include <memory>
#include <string>

/**
 * @class SkinnedMeshRendererComponent
 * @brief エディタ対応のスキニングメッシュ描画コンポーネント。
 * 低レイヤーの AnimatedMeshObject をラップし、インスペクタからモデルを指定できるようにします。
 */
class SkinnedMeshRendererComponent : public Component {
public:
    SkinnedMeshRendererComponent();
    ~SkinnedMeshRendererComponent() override;

    void Initialize() override;
    void Update() override;
    void Draw() override;
    
    void OnRegisterProperties() override;

    void LoadModel(const std::string& filename);
    
    nlohmann::json Serialize() override;
    void Deserialize(const nlohmann::json& j) override;
    
    // アニメーターや他のロジックからポーズを流し込むための窓口
    AnimatedMeshObject* GetRawObject() { return animatedMesh_.get(); }
    void SetShowDebugBones(bool show) { showDebugBones_ = show; }

private:
    std::unique_ptr<AnimatedMeshObject> animatedMesh_;
    std::string modelFilename_ = "sample/walk.gltf";
    bool showDebugBones_ = false;
};
