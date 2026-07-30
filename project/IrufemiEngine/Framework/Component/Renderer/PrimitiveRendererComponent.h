#pragma once
#include "../Component.h"
#include <memory>
#include <string>
#include "Engine/Core/Type/PrimitiveType.h"
#include "Engine/Core/Shape/Sphere.h"

// 前方宣言
class Primitive3DObject;
class TransformComponent;

class PrimitiveRendererComponent : public Component {
public:
    PrimitiveRendererComponent();
    ~PrimitiveRendererComponent() override;

    void Initialize() override;
    void Update() override;
    void Draw() override;
    
    bool CanUpdateInEditMode() const override { return true; }

    IRenderable* GetRenderable() override { return reinterpret_cast<IRenderable*>(primitive_.get()); }
    
    // エディタのRaycast用
    Irufemi::Sphere GetWorldSphere() const;
    bool Raycast(const Irufemi::Ray& ray, float& outDistance) const override;

#ifdef EditorMode
    friend class PrimitiveRendererComponentEditor;
#endif

    std::string GetComponentName() const override { return "PrimitiveRendererComponent"; }
    nlohmann::json Serialize() override;
    void Deserialize(const nlohmann::json& j) override;

    // プロパティ操作
    void SetShape(Irufemi::PrimitiveType type);
    void SetColor(const Irufemi::Vector4& color);
    void SetTexture(const std::string& texturePath);
    void SetEnableLighting(bool enable);
    void SetLightingMode(int mode);
    void SetMetallic(float metallic);
    void SetRoughness(float roughness);
    void SetAlphaReference(float alphaRef);
    void SetUseClampSampler(int32_t useClamp);

private:
    void RebuildMesh();

private:
    std::unique_ptr<Primitive3DObject> primitive_;
    TransformComponent* transform_ = nullptr;
    int currentTypeIndex_ = 2; // デフォルトは Cube (2)

    // --- メッシュ生成用パラメータ ---
    // 汎用
    float radius_ = 1.0f;
    int subdivisions_ = 16;
    float height_ = 1.0f;
    
    // Irufemi::Cylinder / Cone 用
    float topRadius_ = 1.0f;
    float bottomRadius_ = 1.0f;
    bool hasTop_ = true;
    bool hasBottom_ = true;

    // Torus 用
    float torusMajorRadius_ = 1.0f;
    float torusMinorRadius_ = 0.3f;
    int torusMajorSegments_ = 32;
    int torusMinorSegments_ = 16;
};
