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

    // --- 基本サイクル関数 ---

    /**
     * @brief シーンの初期化。継承先で必ず基底クラスの Initialize を呼んでください。
     */
    virtual void Initialize(IrufemiEngine* engine) override;
    
    /**
     * @brief 毎フレームの更新。継承先はこれを呼び出すことで、カメラ等の共通更新が行われます。
     */
    virtual void Update() override;
    
    /**
     * @brief 毎フレームの描画処理（通常は継承先で実装します）
     */
    virtual void Draw() override {}

    // --- ライフサイクル関数 ---
    
    /**
     * @brief シーンの終了処理。リソースの明示的な解放などを行います。
     */
    virtual void Finalize() override {}

    /**
     * @brief シーンが最前面でアクティブになった時に呼ばれます。
     */
    virtual void OnEnter() override {}

    /**
     * @brief シーンが破棄される直前、または完全に非アクティブになる時に呼ばれます。
     */
    virtual void OnExit() override {}

    /**
     * @brief 上に別のシーンがPushされ、このシーンがバックグラウンドに回った時に呼ばれます。
     */
    virtual void OnSuspend() override {}

    /**
     * @brief 上のシーンがPopされ、このシーンが再び最前面に復帰した時に呼ばれます。
     */
    virtual void OnResume() override {}

    // --- デバッグ機能 ---
    
    /**
     * @brief 共通のデバッグタブ描画。
     */
    virtual void DrawDebugTab() override;

protected:
    IrufemiEngine* engine_ = nullptr;

    // --- コア機能 ---
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
