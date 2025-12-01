#pragma once

#include <cstdint>
#include <vector>
#include <d3d12.h>
#include <wrl.h>
#include <memory>
#include <string>

#include "Application/camera/Camera.h"
#include "source/D3D12ResourceUtil.h"

// 前方宣言
class TextureManager;
class DrawManager;
class DebugUI;

class CubeClass {
protected:
    // 中心位置 + サイズ（幅, 高さ, 奥行）
    Vector3 center_{ 0.0f, 0.0f, 0.0f };
    float width_ = 1.0f;  // 横（X）
    float height_ = 1.0f; // 縦（Y）
    float depth_ = 1.0f;  // 奥行（Z）

    // D3D12リソース
    std::unique_ptr<D3D12ResourceUtil> resource_ = nullptr;

    int selectedTextureIndex_ = 0;

    // ポインタ参照（静的）
    static TextureManager* textureManager_;
    static DrawManager* drawManager_;
    static DebugUI* ui_;

    Camera* camera_ = nullptr;

public:
    CubeClass() = default;
    ~CubeClass() = default;

    // 新シグネチャ: depth を明示的に受け取るオーバーロード
    void Initialize(Camera* camera, float width = 1.0f, float height = 1.0f, float depth = 1.0f, const std::string& textureName = "resources/uvChecker.png");

    void Update();
    void Draw();
    void Debug(const char* cubeName = " ");

    // Getters / Setters
    D3D12ResourceUtil* GetD3D12Resource() { return resource_.get(); }
    void SetCenter(const Vector3& c) { center_ = c; }

    // SetSize: 互換性維持用と depth を受け取るオーバーロード
    void SetSize(float width, float height) { SetSize(width, height, width); }
    void SetSize(float width, float height, float depth);

    static void SetTextureManager(TextureManager* texM) { textureManager_ = texM; }
    static void SetDrawManager(DrawManager* drawM) { drawManager_ = drawM; }
    static void SetDebugUI(DebugUI* ui) { ui_ = ui; }
};