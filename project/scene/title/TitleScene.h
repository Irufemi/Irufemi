#pragma once

#include "../IScene.h"

#include "audio/Bgm.h"
#include "audio/Se.h"
#include "math/shape/LinePrimitive.h"
#include "2D/Sprite.h"
#include "3D/ObjClass.h"
#include "camera/Camera.h"
#include "camera/DebugCamera.h"
#include "3D/PointLightClass.h"
#include "3D/SpotLightClass.h"
#include "3D/CylinderClass.h"
#include <memory>
#include <vector>
#include "../contents/CommonVisual.h"  // 追加

/// <summary>
/// タイトル
/// </summary>
class TitleScene : public IScene {

private :
    // --- タイトル1文字の制御（画面座標で作成→ワールド座標で挙動） ---
    struct TitleLetter {
        // 表示オブジェクト（所有は TitleScene）
        ObjClass* obj = nullptr;

        // スクリーン空間（初期レイアウト用）
        Vector2   startScreen{};         // 開始（戻り）位置 [px]
        Vector2   screenPos{};           // 現在位置 [px]
        Vector2   radiusPx{ 18.0f,18.0f }; // 衝突判定用の円半径 [px]
        float     fallSpeedPx = 120.0f;  // 落下速度 [px/s] （画面単位）

        // 挙動ステート（共通）
        bool      isFalling = false;
        bool      isReturning = false;
        float     returnT = 0.0f;
        float     returnSpeed = 3.0f;    // 戻り補間速度（Lerp係数）

        // ワールド空間データ（初回変換で設定）
        Vector3   worldPos{};        // 現在のワールド座標
        Vector3   worldStartPos{};   // 元の開始ワールド座標（戻り先）
        float     worldFallSpeed = 0.0f; // ワールド単位の落下速度 [world/s]
        float     worldRadius = 0.0f;    // ワールド単位の当たり半径

        void ResetToStart();
        void StartFall();

        // ワールド座標での更新（毎フレーム呼ぶ）
        void UpdatePerFrameWorld(float dt, const Segment2D& groundWorld, float groundRadiusWorld);
    };

public: // メンバ関数
    void Initialize(IrufemiEngine* engine) override;
    void Update() override;
    void Draw() override;

private: // メンバ変数
    IrufemiEngine* engine_ = nullptr;

    std::unique_ptr<Camera> camera_ = nullptr;
    std::unique_ptr<DebugCamera> debugCamera_ = nullptr;

    std::unique_ptr<Se> se_select = nullptr;
    std::unique_ptr<Bgm> bgm_ = nullptr;

    std::unique_ptr<ObjClass> text_dan_1 = nullptr;
    std::unique_ptr<ObjClass> text_dan_2 = nullptr;
    std::unique_ptr<ObjClass> text_ri = nullptr;
    std::unique_ptr<ObjClass> text_sa = nullptr;
    std::unique_ptr<ObjClass> text_i = nullptr;
    std::unique_ptr<ObjClass> text_ku = nullptr;
    std::unique_ptr<ObjClass> text_ru = nullptr;

    std::unique_ptr<Sprite> text_press = nullptr;

    bool isDraw_text_press;

    std::unique_ptr<PointLightClass> pointLight_ = nullptr;
    std::unique_ptr<SpotLightClass> spotLight_ = nullptr;

    bool debugMode = false;

    // --- 地面（スクリーン上の線分を初回でワールドへ変換して固定） ---
    Segment2D titleGroundLeft_{ // screen coords (base)
        Vector2{-500.0f, 550.0f},
        Vector2{250.0f, 700.0f}
    };
    Segment2D titleGroundWorld_{};

    // 追加（右床：GameScene と同レイアウト）
    Segment2D titleGroundRight_{        // screen coords
        Vector2{250.0f, 700.0f},
        Vector2{1000.0f, 550.0f}
    };
    Segment2D titleGroundWorldRight_{}; // world coords
    std::unique_ptr<CylinderClass> groundObjRight_ = nullptr;

    // 床描画（左右）
    std::unique_ptr<CylinderClass> groundObj_ = nullptr;         // 左
    float groundThicknessPx_    = Visual::kGroundThicknessPx;    // 共通化
    float groundVisualExtendPx_ = 1600.0f;

    // カメラトランジション
    enum class TransPhase { None, ZoomOut, ZoomIn, WipeIn }; // ← WipeIn 追加
    TransPhase transPhase_ = TransPhase::None;
    float transTimer_ = 0.0f;
    float outDuration_ = 0.6f;
    float inDuration_  = 0.6f;

    // ズーム用Z
    float zStart_ = -10.0f;
    float zMid_   = -30.0f;
    float zEnd_   = -10.0f;

    // パン用X（ZoomInで左寄り→中央へ）
    float xStart_ = 0.0f; // 遷移開始時のX
    float xEnd_   = 0.0f; // 左右床の中点X

    float groundRadiusWorld_ = 0.0f;
    bool  groundConverted_   = false;

    std::vector<TitleLetter> letters_;
    float  fallInterval_ = 1.0f;
    float  fallTimer_    = 0.0f;
    size_t fallIndex_    = 0;
};