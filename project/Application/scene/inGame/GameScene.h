#pragma once

#include "scene/IScene.h"

#include <memory>

#include "3D/TriangleClass.h"
#include "2D/Sprite.h"
#include "2D/Circle2D.h"
#include "2D/NumberText.h"
#include "2D/TimeDisplay.h"
#include "3D/SphereClass.h"
#include "3D/ObjClass.h"
#include "3D/Region.h"
#include "3D/particle/ParticleSystem.h"
#include "3D/CylinderClass.h"
#include "3D/PointLightClass.h"
#include "3D/SpotLightClass.h"
#include "audio/Bgm.h"

#include "camera/CameraController.h"

#include "contents/player/Player.h"
#include "contents/MapChipField.h"

// 前方宣言
class IrufemiEngine;

class InputManager;

class Camera;
class DebugCamera;


/// <summary>
/// ゲーム
/// </summary>
class GameScene : public IScene {
private: // 関数

    void GenerateBlocks();

private: // 変数

    // カメラコントローラー
    std::unique_ptr<CameraController> cameraController_ = nullptr;

    /// マップチップフィールド
    std::unique_ptr<MapChipField> mapChipField_ = nullptr;

    /// ブロック

    // ブロック群
    std::unique_ptr<class Region> blocks_ = nullptr;
    // ワールドトランスフォーム(ブロック)
    std::vector<std::vector<Transform*>> worldtransformBlocks_;

    /// 自キャラ

    // 自キャラ
    std::shared_ptr<Player> player_ = nullptr;
    // 3Dモデルデータ(自キャラ)
    std::unique_ptr<ObjClass> modelplayer_ = nullptr;

private: // メンバ変数(システム)

    // カメラ
    std::unique_ptr<Camera> camera_ = nullptr;

    // デバッグカメラ
    std::unique_ptr<DebugCamera> debugCamera_ = nullptr;

    std::unique_ptr<PointLightClass> pointLight_ = nullptr;

    std::unique_ptr<SpotLightClass> spotLight_ = nullptr;

    int loadTexture = false;

    bool debugMode = false;

    // ポインタ参照

    // エンジン
    IrufemiEngine* engine_ = nullptr;

public: // メンバ関数

    // デストラクタ
    ~GameScene();

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize(IrufemiEngine* engine) override;

    /// <summary>
    /// 更新
    /// </summary>
    void Update() override;

    /// <summary>
    /// 描画
    /// </summary>
    void Draw() override;
};