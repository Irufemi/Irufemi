#pragma once
#include "../Component.h"
#include <memory>
#include <string>
#include "Engine/Core/Shape/Sphere.h"

// 前方宣言
class StaticModelObject;
class TransformComponent;

class MeshRendererComponent : public Component {
public:
    MeshRendererComponent();
    ~MeshRendererComponent() override;

    // 初期化時にモデルファイル名を指定
    void LoadModel(const std::string& filename);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    
    bool CanUpdateInEditMode() const override { return true; }

    void SetEnableEffectMask(bool enable);
    void SetCustomEffectType(int32_t type);
    void SetCustomEffectParam(float param);

    void SetVisible(bool visible) { isVisible_ = visible; }
    bool IsVisible() const { return isVisible_; }

    IRenderable* GetRenderable() override { return reinterpret_cast<IRenderable*>(obj_.get()); }
    
    // エディタのRaycast用
    Sphere GetWorldSphere() const;
    bool Raycast(const Ray& ray, float& outDistance) const override;

    std::string GetComponentName() const override { return "MeshRendererComponent"; }
    nlohmann::json Serialize() override;
    void Deserialize(const nlohmann::json& j) override;

    /**
     * @brief 現在読み込まれているモデル名を取得します。
     * @return モデル名
     */
    const std::string& GetModelName() const { return modelName_; }

#ifdef EditorMode
    friend class MeshRendererComponentEditor;
#endif

private:
    std::unique_ptr<StaticModelObject> obj_;                 ///< 実際の描画を担う既存クラス
    TransformComponent* transform_ = nullptr;       ///< 親のTransform情報（キャッシュ）
    std::string modelName_ = "";           ///< 読み込むモデル名
    bool castShadows_ = true;
    bool isVisible_ = true;
};
