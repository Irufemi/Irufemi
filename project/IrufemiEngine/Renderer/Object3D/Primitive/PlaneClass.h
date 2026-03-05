#pragma once

#include <vector>
#include <array>
#include <d3d12.h>
#include <string>
#include <memory>

#include "math/shape/Plane.h"
#include "source/D3D12ResourceUtil.h"
#include "Application/camera/Camera.h"
#include <wrl.h>

// 前方宣言
class TextureManager;
class DrawManager;
class DebugUI;

class PlaneClass
{
private: //メンバ変数

    Plane info_{};

    std::unique_ptr<D3D12ResourceUtil> resource_ = nullptr;

    int selectedTextureIndex_ = 0;

    // ポインタ参照

    Camera* camera_ = nullptr;

    static TextureManager* textureManager_;

    static DrawManager* drawManager_;

    static DebugUI* ui_;

public: //メンバ関数
    //デストラクタ
    ~PlaneClass() = default;

    //初期化
    void Initialize(Camera* camera, const std::string& textureName = "resources/uvChecker.png");
    //更新
    void Update();

    // 描画
    void Draw();

    // デバッグ
    void Debug(const char* planeName = "");

    //ゲッター
    D3D12ResourceUtil* GetD3D12Resource() { return this->resource_.get(); }

    // Planeの情報を取得
    Plane GetInfo() const { return info_; }

    void SetTransform(const Transform& transform) { resource_->transform_ = transform; }
    Transform GetTransform()const { return resource_->transform_; }

    void SetTranslate(const Vector3& translate) { resource_->transform_.translate = translate; }
    Vector3 GetTranslate()const { return resource_->transform_.translate; }

    void SetScale(const Vector3& scale) { resource_->transform_.scale = scale; }
    Vector3 GetScale()const { return resource_->transform_.scale; }

    void SetRotate(const Vector3& rotate) { resource_->transform_.rotate = rotate; }
    Vector3 GetRotate()const { return resource_->transform_.rotate; }

    static void SetTextureManager(TextureManager* texM) { textureManager_ = texM; }
    static void SetDrawManager(DrawManager* drawM) { drawManager_ = drawM; }
    static void SetDebugUI(DebugUI* ui) { ui_ = ui; }
};

