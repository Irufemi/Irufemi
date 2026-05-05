#pragma once
#include "IScene.h"
#include <memory>
#include <vector>
#include <cstdint>

// 前方宣言
class IrufemiEngine;
class CameraManager;
class DebugCamera;
struct DirectionalLight;
struct PointLight;
struct SpotLight;
struct AreaLight;

/**
 * @class BaseScene
 * @brief すべてのゲームシーンの基底クラス。必須オブジェクト（カメラ、ライト等）を統合管理する。
 */
class BaseScene : public IScene {
public:
    BaseScene();
    virtual ~BaseScene();

    // 継承先で必ず呼ぶ
    virtual void Initialize(IrufemiEngine* engine) override;
    
    // 継承先はこれを呼び出すことで、カメラ等の共通更新が行われる
    virtual void Update() override;
    
    // 描画処理（通常は継承先で実装）
    virtual void Draw() override {}
    
    // 共通のデバッグタブ描画
    virtual void DrawDebugTab() override;

protected:
    IrufemiEngine* engine_ = nullptr;

    // --- コア機能 ---
    std::unique_ptr<CameraManager> cameraManager_;
    std::unique_ptr<DebugCamera> debugCamera_;
    bool isDebugCameraMode_ = false;

    // --- ライティング ---
    std::unique_ptr<DirectionalLight> directionalLight_;
    std::vector<std::unique_ptr<PointLight>> pointLights_;
    std::vector<std::unique_ptr<SpotLight>> spotLights_;
    std::vector<std::unique_ptr<AreaLight>> areaLights_;

    // --- フレームデータの自動送信 ---
    void SubmitFrameData();

    // ── 入力ヘルパ ──
    // InputManager をラップした安全なヘルパー
    bool DownVK(uint8_t vk) const;
    bool PressedVK(uint8_t vk) const;
    bool ReleasedVK(uint8_t vk) const;

    bool DownDIK(uint8_t dik) const;
    bool PressedDIK(uint8_t dik) const;
    bool ReleasedDIK(uint8_t dik) const;

    // 互換性のため（既存の IsKeyPressed 等も呼び出しやすくする）
    bool IsKeyDown(uint8_t vk) const { return DownVK(vk); }
    bool IsKeyPressed(uint8_t vk) const { return PressedVK(vk); }
    bool IsButtonDown(unsigned short button) const;
    bool IsButtonPressed(unsigned short button) const;
};
