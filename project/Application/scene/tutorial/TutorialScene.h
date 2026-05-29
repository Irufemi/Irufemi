#pragma once

#include "Framework/BaseScene.h"
#include <memory>
#include <vector>
#include "Engine/Core/Math/Vector3.h"

class IrufemiEngine;
class InputManager;
class Camera;
class Player;
class Enemy;
class Field;
class Skydome;
struct PointLight;
class Sprite;
class DynamicArenaLight;

enum class TutorialPhase {
    MoveWASD,
    Dodge,
    MeleeAttack,
    GunAttack,
    KarakuriCharge,
    EnhancedDodge,
    MissileAttack,
    MissileHitFocus,      // ミサイル着弾後の引きカメラ
    BuildingSpawnFocus,
    BuildingReadyFocus,   // ビル生成完了後の少し引きカメラ（SPACE待ち）
    BuildingAttack,
    BuildingHitFocus,     // ビル激突後の引きカメラ
    PartsExplanation,
    ViewSwitch,
    Done
};

class TutorialScene : public BaseScene {
public:
    TutorialScene();
    ~TutorialScene() override;

    void Initialize(IrufemiEngine* engine) override;
    void Update() override;
    void Draw() override;
    bool IsCursorVisible() const override { return false; }

private:
    void CheckAllCollisions();
    void CheckPlayerToEnemyCollisions();
    void CheckEnemyToPlayerCollisions();
    void CheckPlayerBuildingCollisions();
    void CheckFlyingBuildingsVsEnemyCollisions();

    // チュートリアル状態の更新と描画
    void UpdateTutorialState();

private:
    bool isFirstDebug_ = true;

    // ゲームオブジェクト
    std::unique_ptr<Player> player_ = nullptr;
    std::unique_ptr<Enemy> boss_ = nullptr;
    std::unique_ptr<Field> field_ = nullptr;
    std::unique_ptr<Skydome> skydome_ = nullptr;
    std::unique_ptr<DynamicArenaLight> dynamicArenaLight_ = nullptr;

    bool isCollisionEnabled_ = true;

    TutorialPhase currentPhase_ = TutorialPhase::MoveWASD;
    Vector3 spawnedBuildingPos_ = {};
    float cinematicTimer_ = 0.0f; // 演出タイマー

    // 入力監視フラグ
    bool hasPressedW_ = false;
    bool hasPressedA_ = false;
    bool hasPressedS_ = false;
    bool hasPressedD_ = false;
    
    // 進行フラグ
    bool hasHitMelee_ = false;
    bool hasHitGun_ = false;
    bool hasHitMissile_ = false;
    bool hasBuildingHitEnemy_ = false;

    std::unique_ptr<Sprite> tutorialUISprites_[10];
    
    // WASDキーUI用
    std::unique_ptr<Sprite> keyWSprite_;
    std::unique_ptr<Sprite> keyASprite_;
    std::unique_ptr<Sprite> keySSprite_;
    std::unique_ptr<Sprite> keyDSprite_;

    std::unique_ptr<Sprite> keySpaceSprite_;
    
    // ESCキーUI用
    std::unique_ptr<Sprite> keyEscSprite_;
};
