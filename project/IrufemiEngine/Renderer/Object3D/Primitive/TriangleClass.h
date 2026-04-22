#pragma once

#include <vector>
#include <array>
#include <d3d12.h>
#include <string>

#include "Engine/Core/Shape/Triangle.h"
#include "Renderer/Object3D/Object3DResource.h"
#include "Application/camera/Camera.h"
#include <wrl.h>
#include <memory>

// 前方宣言
class TextureManager;
class DrawManager;
class DebugUI;

class TriangleClass {
private:
    std::unique_ptr<Object3DResource> resource_ = nullptr;
    int selectedTextureIndex_ = 0;

    Camera* camera_ = nullptr;

    // 行列更新の最適化用
    bool isDirty_ = true;
    bool isCullingEnabled_ = true;
    Matrix4x4 lastViewMatrix_ = {};
    Matrix4x4 lastProjectionMatrix_ = {};

    static TextureManager* textureManager_;
    static DrawManager* drawManager_;
    static DebugUI* ui_;

public:
    ~TriangleClass() = default;

    void Initialize(Camera* camera, const std::string& textureName = "resources/uvChecker.png");
    void Update();
    void Draw();
    void Debug(const char* triangleName = "");

    // ゲッター/セッター
    Object3DResource* GetD3D12Resource() { return resource_.get(); }

    void SetTransform(const Transform& transform) { if (resource_) resource_->transform_ = transform; isDirty_ = true; }
    const Transform& GetTransform() const { return resource_ ? resource_->transform_ : sDefaultTransform_; }

    void SetTranslate(const Vector3& translate) { if (resource_) resource_->transform_.translate = translate; isDirty_ = true; }
    Vector3 GetTranslate() const { return resource_ ? resource_->transform_.translate : Vector3{}; }

    void SetScale(const Vector3& scale) { if (resource_) resource_->transform_.scale = scale; isDirty_ = true; }
    Vector3 GetScale() const { return resource_ ? resource_->transform_.scale : Vector3{}; }

    void SetRotate(const Vector3& rotate) { if (resource_) resource_->transform_.rotate = rotate; isDirty_ = true; }
    Vector3 GetRotate() const { return resource_ ? resource_->transform_.rotate : Vector3{}; }

    void SetColor(const Vector4& color) { if (resource_ && resource_->GetMaterialData()) resource_->GetMaterialData()->color = color; }
    Vector4 GetColor() const { return resource_ && resource_->GetMaterialData() ? resource_->GetMaterialData()->color : Vector4{}; }

    void SetAlphaReference(float alphaRef) { if (resource_ && resource_->GetMaterialData()) resource_->GetMaterialData()->alphaReference = alphaRef; }
    void SetUseClampSampler(int32_t useClamp) { if (resource_ && resource_->GetMaterialData()) resource_->GetMaterialData()->useClampSampler = useClamp; }

    static void SetTextureManager(TextureManager* texM) { textureManager_ = texM; }
    static void SetDrawManager(DrawManager* drawM) { drawManager_ = drawM; }
    static void SetDebugUI(DebugUI* ui) { ui_ = ui; }
    void SetCullingEnabled(bool enabled) { isCullingEnabled_ = enabled; }
    bool IsCullingEnabled() const { return isCullingEnabled_; }

private:
    static inline Transform sDefaultTransform_{};
};
