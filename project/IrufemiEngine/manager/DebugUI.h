#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <array>          
#include <cstddef>        
#include "math/BlendMode.h"
#include "engine/directX/PSOManager.h"


// 前方宣言
class TextureManager;
class SceneManager;
class D3D12ResourceUtil;
class D3D12ResourceUtilParticle;
struct Transform;
struct Matrix4x4;
struct DirectionalLight;
struct Material;
struct Sphere;
class DirectXCommon;
struct ParticleMaterial; // 追加: Particle専用マテリアル前方宣言
struct ObjMaterial;

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
    static void DebugTransform(Transform& transform);

    static void DebugTransform2D(Transform& transform);


    static void TextTransform(Transform& transform, const char* name = "");

    // Material
    static void DebugMaterialBy3D(Material* material);
    
    // Material
    static void DebugMaterialBy2D(Material* material);

    // ObjMaterialのデバッグ表示
    static void DebugObjMaterial(ObjMaterial* material, const char* unique_id = "");

    // Particle 専用マテリアルのデバッグ表示
    static void DebugMaterialParticle(ParticleMaterial* material);

    // 画像
    void DebugTexture(D3D12ResourceUtil * resource_,int & selectedTextureIndex_);
    void DebugTexture(D3D12ResourceUtilParticle* resource, int& selectedTextureIndex);

    // DirectionalLight
    static void DebugDirectionalLight(DirectionalLight* directionalLightData);

    // UvTransform
    static void DebugUvTransform(Transform& uvTransform);

    // UvTransform
    static void DebugUvTransform(Matrix4x4& uvTransform);

    // Sphere
    static void DebugSphereInfo(Sphere& sphere);

    // FPS/FrameTime オーバーレイ
    void FPSDebug();

    // シーンセレクタ
    void DebugSceneSelector(SceneManager* sm);

    // PSO設定（ブレンド、深度、カリング）のデバッグUI
    static void DebugPsoSettings(
        BlendMode* blendMode,
        PSOManager::DepthWrite* depthWrite,
        PSOManager::CullMode* cullMode,
        const char* unique_id = "##PsoSettings"
    );
};

