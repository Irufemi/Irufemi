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
     * @brief Renderable を取得する。
     * @return 取得された Renderable
     */
    IRenderable* GetRenderable() override { return reinterpret_cast<IRenderable*>(primitive_.get()); }
    
    // エディタのRaycast用
    /**
     * @brief WorldSphere を取得する。
     * @return 取得された WorldSphere
     */
    Irufemi::Sphere GetWorldSphere() const;
    /**
     * @brief Raycast を実行する。
     */
    bool Raycast(const Irufemi::Ray& ray, float& outDistance) const override;

#ifdef EditorMode
    friend class PrimitiveRendererComponentEditor;
#endif

    /**
     * @brief ComponentName を取得する。
     * @return 取得された ComponentName
     */
    std::string GetComponentName() const override { return "PrimitiveRendererComponent"; }
    /**
     * @brief Serialize を実行する。
     */
    nlohmann::json Serialize() override;
    /**
     * @brief Deserialize を実行する。
     */
    void Deserialize(const nlohmann::json& j) override;

    // プロパティ操作
    /**
     * @brief Shape を設定する。
     * @param[in] type 設定する Shape の値
     */
    void SetShape(Irufemi::PrimitiveType type);
    /**
     * @brief Color を設定する。
     * @param[in] color 設定する Color の値
     */
    void SetColor(const Irufemi::Vector4& color);
    /**
     * @brief Texture を設定する。
     * @param[in] texturePath 設定する Texture の値
     */
    void SetTexture(const std::string& texturePath);
    /**
     * @brief EnableLighting を設定する。
     * @param[in] enable 設定する EnableLighting の値
     */
    void SetEnableLighting(bool enable);
    /**
     * @brief LightingMode を設定する。
     * @param[in] mode 設定する LightingMode の値
     */
    void SetLightingMode(int mode);
    /**
     * @brief Metallic を設定する。
     * @param[in] metallic 設定する Metallic の値
     */
    void SetMetallic(float metallic);
    /**
     * @brief Roughness を設定する。
     * @param[in] roughness 設定する Roughness の値
     */
    void SetRoughness(float roughness);
    /**
     * @brief AlphaReference を設定する。
     * @param[in] alphaRef 設定する AlphaReference の値
     */
    void SetAlphaReference(float alphaRef);
    /**
     * @brief UseClampSampler を設定する。
     * @param[in] useClamp 設定する UseClampSampler の値
     */
    void SetUseClampSampler(int32_t useClamp);

private:
    /**
     * @brief RebuildMesh を実行する。
     */
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
