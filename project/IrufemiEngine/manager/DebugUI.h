#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <array>          
#include <cstddef>        


// 前方宣言
class TextureManager;
class SceneManager;
class D3D12ResourceUtil;
class D3D12ResourceUtilParticle;
struct Transform;
struct DirectionalLight;
struct Material;
struct Sphere;
class DirectXCommon;

#ifdef USE_IMGUI

#include "imgui/imgui.h"

#endif // USE_IMGUI


class DebugUI{
private: // メンバ変数

    // ポインタ参照(非所有)

    DirectXCommon* dxCommon_ = nullptr;

    TextureManager* textureManager_ = nullptr;

    // ★追加: パフォーマンス履歴
    static constexpr size_t kPerfHistoryCount_ = 240;          // 約4秒分
    std::array<float, kPerfHistoryCount_> frameTimeHistory_{}; // ms
    size_t historyIndex_ = 0;
    bool historyFilled_ = false;

    // ★内部計算キャッシュ
    float cachedAvgMs_ = 0.0f;
    float cachedMinMs_ = 0.0f;
    float cachedMaxMs_ = 0.0f;
    float cachedP99Ms_ = 0.0f;   // 99th percentile frame time (≒ 1% worst)
    float cachedFps_ = 0.0f;
    void UpdatePerfStats_(float newFrameMs); // ★集計用内部関数

public: // メンバ関数

    // 初期化
    void Initialize(HWND hwnd, DirectXCommon* dxCommon);

    // TextureManagerをセット
    void SetTextureManager(TextureManager* textureManager) { this->textureManager_ = textureManager; }

    // 終了処理
    void Shutdown();
#ifdef USE_IMGUI
    static LRESULT WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif // USE_IMGUI

    // フレーム開始
    void FrameStart();

    // 描画処理に入る前にコマンドを積む
    void QueueDrawCommands();

    // 描画処理が終わったタイミングでコマンドを積む
    void QueuePostDrawCommands();

    // Transform
    void DebugTransform(Transform& transform);

    void DebugTransform2D(Transform& transform);


    void TextTransform(Transform& transform, const char* name = "");

    // Material
    void DebugMaterialBy3D(Material* material);

    // Material
    void DebugMaterialBy2D(Material* material);

    // 画像
    void DebugTexture(D3D12ResourceUtil * resource_,int & selectedTextureIndex_);
    void DebugTexture(D3D12ResourceUtilParticle* resource, int& selectedTextureIndex);

    // DirectionalLight
    void DebugDirectionalLight(DirectionalLight* directionalLightData);

    // UvTransform
    void DebugUvTransform(Transform& uvTransform);

    // Sphere
    void DebugSphereInfo(Sphere& sphere);

    // FPS/FrameTime オーバーレイ
    void FPSDebug();

    // シーンセレクタ
    void DebugSceneSelector(SceneManager* sm);

};

