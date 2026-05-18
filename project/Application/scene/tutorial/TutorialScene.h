#pragma once

#include "Framework/BaseScene.h"
#include <memory>
#include <vector>

class IrufemiEngine;
class InputManager;
class Camera;
class Player;
class Enemy;
class Field;
class Skydome;
struct PointLight;
class Sprite;

enum class TutorialPhase {
    MoveWASD,
    Dodge,
    MeleeAttack,
    GunAttack,
    KarakuriCharge,
    EnhancedDodge,
    MissileAttack,
    BuildingAttack,
    PartsExplanation,
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
    void DrawDebugTab() override;

private:
    void UpdateDynamicLights();
    void CheckAllCollisions();
    void CheckPlayerToEnemyCollisions();
    void CheckEnemyToPlayerCollisions();
    void CheckFlyingPartsCollisions();
    void CheckPlayerBuildingCollisions();
    void CheckEnemyBuildingCollisions();
    void CheckFlyingPartsBuildingCollisions();
    void CheckFlyingBuildingsVsEnemyCollisions();
    void CheckFlyingBuildingsVsBuildingsCollisions();

    // チュートリアル状態の更新と描画
    void UpdateTutorialState();
    void DrawTutorialUI();

private:
    bool isFirstDebug_ = true;

    // ゲームオブジェクト
    std::unique_ptr<Player> player_ = nullptr;
    std::unique_ptr<Enemy> boss_ = nullptr;
    std::unique_ptr<Field> field_ = nullptr;
    std::unique_ptr<Skydome> skydome_ = nullptr;

    bool isCollisionEnabled_ = true;

    TutorialPhase currentPhase_ = TutorialPhase::MoveWASD;

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

    // チュートリアル用のメッセージ
    const char* currentInstruction_ = "";
};
