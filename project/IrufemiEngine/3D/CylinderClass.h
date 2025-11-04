#pragma once

#include <cstdint>

#include "camera/Camera.h"
#include <vector>
#include <d3d12.h>
#include <wrl.h>
#include <memory>
#include "source/D3D12ResourceUtil.h"
// Vector3 等の型を取り込むために既存の shape を経由
#include "math/shape/Sphere.h"

// 前方宣言
class TextureManager;
class DrawManager;
class DebugUI;

struct Cylinder {
    Vector3 center{ 0.0f, 0.0f, 0.0f };
    float   radius = 1.0f; // XZ 方向の半径
    float   height = 1.0f; // Y 方向の高さ
};

class CylinderClass {
protected: // メンバ変数

    Cylinder info_{};

    const float pi_ = 3.141592654f;

    // 周方向分割数
    const uint32_t kSubdivision_ = 16;
    // 高さ方向分割数（側面）
    const uint32_t kHeightSubdivision_ = 1;

    // 周方向 1 セグメントあたりの角度
    const float kThetaEvery_ = pi_ * 2.0f / static_cast<float>(kSubdivision_);

    // D3D12 リソース
    std::unique_ptr<D3D12ResourceUtil> resource_ = nullptr;

    int selectedTextureIndex_ = 0;

    // ポインタ参照（非所有）
    static TextureManager* textureManager_;
    static DrawManager* drawManager_;
    static DebugUI* ui_;
    Camera* camera_ = nullptr;

public: // メンバ関数
    // コンストラクタ
    CylinderClass() {}
    // デストラクタ
    ~CylinderClass() = default;

    // 初期化
    void Initialize(Camera* camera, const std::string& textureName = "resources/uvChecker.png");

    // 更新
    void Update(const char* cylinderName = " ");

    // 描画
    void Draw();

    // 補助
    D3D12ResourceUtil* GetD3D12Resource() { return this->resource_.get(); }
    void AddRotateY(float value) { this->resource_->transform_.rotate.y += value; }

    // 情報アクセス
    Cylinder GetInfo() const { return info_; }

    static void SetTextureManager(TextureManager* texM) { textureManager_ = texM; }
    static void SetDrawManager(DrawManager* drawM) { drawManager_ = drawM; }
    static void SetDebugUI(DebugUI* ui) { ui_ = ui; }

    void SetInfo(const Cylinder& info) { info_ = info; }
    void SetCenter(const Vector3& center) { info_.center = center; }
    void SetRadius(float radius) { info_.radius = radius; }
    void SetHeight(float height) { info_.height = height; }
    void SetRotate(const Vector3& rotate) { resource_->transform_.rotate = rotate; }
    void SetColor(const Vector4& color) { resource_->materialData_->color = color; }

private:
    // 上下面キャップを個別に作るヘルパー
    // top == true => +Y 側のキャップ（上蓋）
    // doubleSided == true => 各三角形の逆順も追加して両面にする
    void AddCap(bool top, bool doubleSided = false);
};

