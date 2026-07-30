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

    /**
     * @brief Initialize を実行する。
     */
    void Initialize() override;
    /**
     * @brief Update を実行する。
     */
    void Update() override;
    /**
     * @brief Draw を実行する。
     */
    void Draw() override;
    
    /**
     * @brief CanUpdateInEditMode かどうかを判定する。
     * @return 判定結果 (true/false)
     */
    bool CanUpdateInEditMode() const override { return true; }

    /**
     * @brief OnRegisterProperties を実行する。
     */
    void OnRegisterProperties() override;
    
    /**
     * @brief ComponentName を取得する。
     * @return 取得された ComponentName
     */
    std::string GetComponentName() const override { return "SkinnedMeshRendererComponent"; }
    /**
     * @brief Raycast を実行する。
     */
    bool Raycast(const Irufemi::Ray& ray, float& outDistance) const override;
    /**
     * @brief Renderable を取得する。
     * @return 取得された Renderable
     */
    IRenderable* GetRenderable() override { return animatedMesh_.get(); }

    /**
     * @brief LoadModel を実行する。
     */
    void LoadModel(const std::string& filename);
    
    /**
     * @brief Serialize を実行する。
     */
    nlohmann::json Serialize() override;
    /**
     * @brief Deserialize を実行する。
     */
    void Deserialize(const nlohmann::json& j) override;
    
    // アニメーターや他のロジックからポーズを流し込むための窓口
    /**
     * @brief RawObject を取得する。
     * @return 取得された RawObject
     */
    AnimatedMeshObject* GetRawObject() { return animatedMesh_.get(); }


    // Animatorからのポーズ上書き用
    /**
     * @brief PoseOverride を設定する。
     * @param[in] pose 設定する PoseOverride の値
     */
    void SetPoseOverride(const struct SkeletonPose* pose) { poseOverride_ = pose; }

private:
    std::unique_ptr<AnimatedMeshObject> animatedMesh_;
    std::string modelFilename_ = "";
    std::string currentLoadedFilename_ = "";

    const struct SkeletonPose* poseOverride_ = nullptr;
    
    std::unordered_map<size_t, ObjMaterial> materialOverrides_;

public:
    /**
     * @brief HasMaterialOverride かどうかを判定する。
     * @return 判定結果 (true/false)
     */
    bool HasMaterialOverride(size_t meshIndex) const { return materialOverrides_.find(meshIndex) != materialOverrides_.end(); }
    /**
     * @brief MaterialOverride を取得する。
     * @return 取得された MaterialOverride
     */
    const ObjMaterial* GetMaterialOverride(size_t meshIndex) const {
        auto it = materialOverrides_.find(meshIndex);
        return it != materialOverrides_.end() ? &it->second : nullptr;
    }
    /**
     * @brief MaterialOverrideMutable を取得する。
     * @return 取得された MaterialOverrideMutable
     */
    ObjMaterial* GetMaterialOverrideMutable(size_t meshIndex) {
        auto it = materialOverrides_.find(meshIndex);
        return it != materialOverrides_.end() ? &it->second : nullptr;
    }
    /**
     * @brief MaterialOverride を設定する。
     * @param[in] meshIndex 設定する MaterialOverride の値
     * @param[in] material 設定する MaterialOverride の値
     */
    void SetMaterialOverride(size_t meshIndex, const ObjMaterial& material) {
        materialOverrides_[meshIndex] = material;
    }
    /**
     * @brief RemoveMaterialOverride を実行する。
     */
    void RemoveMaterialOverride(size_t meshIndex) {
        materialOverrides_.erase(meshIndex);
    }
    const std::unordered_map<size_t, ObjMaterial>& GetAllMaterialOverrides() const { return materialOverrides_; }
};
