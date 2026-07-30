#pragma once

#include "../Component.h"
#include "../../../Renderer/Object/3D/AnimationModel/AnimatedMeshObject.h"
#include <memory>
#include <string>
#include <unordered_map>
#include "../../../Resource/Model/Data/ObjModel.h"

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
    
    bool CanUpdateInEditMode() const override { return true; }

    void OnRegisterProperties() override;
    
    std::string GetComponentName() const override { return "SkinnedMeshRendererComponent"; }
    bool Raycast(const Irufemi::Ray& ray, float& outDistance) const override;
    IRenderable* GetRenderable() override { return animatedMesh_.get(); }

    void LoadModel(const std::string& filename);
    
    nlohmann::json Serialize() override;
    void Deserialize(const nlohmann::json& j) override;
    
    // アニメーターや他のロジックからポーズを流し込むための窓口
    AnimatedMeshObject* GetRawObject() { return animatedMesh_.get(); }


    // Animatorからのポーズ上書き用
    void SetPoseOverride(const struct SkeletonPose* pose) { poseOverride_ = pose; }

private:
    std::unique_ptr<AnimatedMeshObject> animatedMesh_;
    std::string modelFilename_ = "";
    std::string currentLoadedFilename_ = "";

    const struct SkeletonPose* poseOverride_ = nullptr;
    
    std::unordered_map<size_t, ObjMaterial> materialOverrides_;

public:
    bool HasMaterialOverride(size_t meshIndex) const { return materialOverrides_.find(meshIndex) != materialOverrides_.end(); }
    const ObjMaterial* GetMaterialOverride(size_t meshIndex) const {
        auto it = materialOverrides_.find(meshIndex);
        return it != materialOverrides_.end() ? &it->second : nullptr;
    }
    ObjMaterial* GetMaterialOverrideMutable(size_t meshIndex) {
        auto it = materialOverrides_.find(meshIndex);
        return it != materialOverrides_.end() ? &it->second : nullptr;
    }
    void SetMaterialOverride(size_t meshIndex, const ObjMaterial& material) {
        materialOverrides_[meshIndex] = material;
    }
    void RemoveMaterialOverride(size_t meshIndex) {
        materialOverrides_.erase(meshIndex);
    }
    const std::unordered_map<size_t, ObjMaterial>& GetAllMaterialOverrides() const { return materialOverrides_; }
};
