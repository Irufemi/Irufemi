#define NOMINMAX
#include "TitleScene.h"

#include "../SceneManager.h"
#include "../SceneName.h"
#include "engine/IrufemiEngine.h"
#include <imgui.h>
#include "../inGame/InGameFunction.h"
#include <algorithm> // std::clamp
#include <cmath> // std::atan2 / std::fabs / std::lerp 等のため

#include "../inGame/ScreenSpace.h"

// 0..1 を滑らかに補間する（Hermite SmoothStep）。必要なら Ease 実装に差し替え可。
static float Smooth01(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
    // もし Ease ファイルに同等の関数があるなら、以下のように中身を置き換え可能:
    // return Ease::SmoothStep(0.0f, 1.0f, t);
    // または Ease::SmootherStep(0.0f, 1.0f, t);
}

// --- TitleLetter 実装（ワールド更新） ---
void TitleScene::TitleLetter::ResetToStart() {
    screenPos   = startScreen;
    isFalling   = false;
    isReturning = false;
    returnT     = 0.0f;
    worldPos = Vector3{ 0.0f, 0.0f, 0.0f };
    worldStartPos = Vector3{ 0.0f, 0.0f, 0.0f };
    worldFallSpeed = 0.0f;
    worldRadius = 0.0f;
}
void TitleScene::TitleLetter::StartFall() {
    if (!isFalling && !isReturning) {
        isFalling = true;
        returnT   = 0.0f;
    }
}

void TitleScene::TitleLetter::UpdatePerFrameWorld(float dt, const Segment2D& groundWorld, float groundRadiusWorld) {
    if (!obj) return;

    if (isFalling) {
        worldPos.y -= worldFallSpeed * dt;

        // 床半径を加えた“膨らませ”衝突判定 + 片面当たり
        CollisionResult r = isCollision(
            Vector2{ worldPos.x, worldPos.y },
            Vector2{ worldRadius + groundRadiusWorld, worldRadius + groundRadiusWorld },
            groundWorld);

        if (r.isColliding && r.normal.y > 0.0f) {
            Vector2 push = Math::Multiply(r.penetration, r.normal);
            worldPos.x += push.x;
            worldPos.y += push.y;

            isFalling   = false;
            isReturning = true;
            returnT     = 0.0f;
        }
    }

    if (isReturning) {
        returnT += dt * returnSpeed;
        worldPos = Math::Add(Math::Multiply(1.0f - returnT, worldPos), Math::Multiply(returnT, worldStartPos));
        Vector2 diff = Math::Subtract(Vector2{ worldStartPos.x, worldStartPos.y }, Vector2{ worldPos.x, worldPos.y });
        if (Math::Length(diff) < 0.01f || returnT >= 1.0f) {
            worldPos = worldStartPos;
            isReturning = false;
            returnT = 0.0f;
        }
    }

    obj->SetPosition(worldPos);
}

// --- 初期化 ---
void TitleScene::Initialize(IrufemiEngine* engine) {
    (void)engine;
    engine_ = engine;

    camera_ = std::make_unique<Camera>();
    camera_->Initialize(engine_->GetClientWidth(), engine_->GetClientHeight());
    camera_->SetTranslate(Vector3{ 0.0f,0.0f,-10.0f });

    // 重要：SetTranslate の後で行列を確実に更新しておく
    camera_->UpdateMatrix();

    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize(engine_->GetInputManager(), engine_->GetClientWidth(), engine_->GetClientHeight());
    debugMode = false;

    pointLight_ = std::make_unique <PointLightClass>();
    pointLight_->Initialize();
    pointLight_->SetPos(Vector3{ 0.0f,-5.0f,0.0f });
    engine_->GetDrawManager()->SetPointLightClass(pointLight_.get());

    spotLight_ = std::make_unique<SpotLightClass>();
    spotLight_->Initialize();
    spotLight_->SetIntensity(0.0f);
    engine_->GetDrawManager()->SetSpotLightClass(spotLight_.get());

    bgm_ = std::make_unique<Bgm>();
    bgm_->Initialize("resources/bgm/BGM_Title.mp3");
    bgm_->PlayFixed();

    se_select = std::make_unique<Se>();
    se_select->Initialize("resources/se/SE_Select.mp3");

    text_dan_1 = std::make_unique<ObjClass>();  text_dan_1->Initialize(camera_.get(), "text_dan_01.obj");
    text_dan_2 = std::make_unique<ObjClass>();  text_dan_2->Initialize(camera_.get(), "text_dan_02.obj");
    text_ri    = std::make_unique<ObjClass>();  text_ri->Initialize(camera_.get(), "text_ri.obj");
    text_sa    = std::make_unique<ObjClass>();  text_sa->Initialize(camera_.get(), "text_sa.obj");
    text_i     = std::make_unique<ObjClass>();  text_i->Initialize(camera_.get(), "text_i.obj");
    text_ku    = std::make_unique<ObjClass>();  text_ku->Initialize(camera_.get(), "text_ku.obj");
    text_ru    = std::make_unique<ObjClass>();  text_ru->Initialize(camera_.get(), "text_ru.obj");

    text_press = std::make_unique<Sprite>();
    text_press->Initialize(camera_.get(), "resources/texture/titleText_press.png");
    text_press->SetAnchor(0.5f, 0.5f);
    text_press->SetPosition(camera_->GetViewportWidth() / 2.0f, camera_->GetViewportHeight() / 2.0f + 200.0f);
    isDraw_text_press = true;

    // 文字プレースホルダ作成
    const int nLetters = 7;
    ObjClass* objs[nLetters] = {
        text_dan_1.get(), text_dan_2.get(),
        text_ri.get(), text_sa.get(), text_i.get(), text_ku.get(), text_ru.get()
    };

    letters_.clear();
    letters_.reserve(nLetters);
    for (int i = 0; i < nLetters; ++i) {
        TitleLetter t;
        t.obj = objs[i];
        t.radiusPx = Vector2{ 18.0f, 18.0f };
        t.ResetToStart();
        letters_.push_back(std::move(t));
    }

    // --- スクリーンで一回レイアウト（Y を画面中心に固定） ---
    {
        float screenW = camera_->GetViewportWidth();
        float screenH = camera_->GetViewportHeight();
        float centerX = screenW * 0.5f;
        // ここを画面中心に固定（要求どおり）
        float baseY = screenH * 0.5f;

        float maxWidth = screenW * 0.40f;
        float spacing = maxWidth / static_cast<float>(nLetters - 1);
        spacing = std::clamp(spacing, 70.0f, 140.0f);

        float startX = centerX - spacing * (static_cast<float>(nLetters - 1) * 0.5f);

        for (int i = 0; i < nLetters; ++i) {
            TitleLetter& t = letters_[i];
            t.startScreen = Vector2{ startX + spacing * static_cast<float>(i), baseY };
            t.screenPos = t.startScreen;
        }
    }

    // --- 地面を先にスクリーン→ワールドに変換して固定 ---
    {
        const float innerOverlapPx = 4.0f;
        float halfVis = groundVisualExtendPx_ * 0.5f;
        float halfThickness = groundThicknessPx_ * 0.5f;

        // 左床（外側延長、内側は継ぎ目 + overlap）
        {
            Vector2 p0 = titleGroundLeft_.origin;
            Vector2 p1 = titleGroundLeft_.end;
            Vector2 d  = Math::Subtract(p1, p0);
            float L = Math::Length(d);
            Vector2 dir = (L > 0.0f) ? Math::Multiply(1.0f / L, d) : Vector2{1.0f, 0.0f};

            Vector2 p0v = Math::Add(p0, Math::Multiply(-halfVis, dir));       // 外側を延長
            Vector2 p1v = Math::Add(p1, Math::Multiply(innerOverlapPx, dir)); // 内側を少し延長して重ねる

            Vector3 w0 = ScreenToWorldOnZ(camera_.get(), p0v, 0.0f);
            Vector3 w1 = ScreenToWorldOnZ(camera_.get(), p1v, 0.0f);
            titleGroundWorld_.origin = Vector2{ w0.x, w0.y };
            titleGroundWorld_.end    = Vector2{ w1.x, w1.y };

            Vector3 center = Math::Multiply(0.5f, Math::Add(w0, w1));
            center.z += 0.1f;
            Vector2 d2 = Vector2{ w1.x - w0.x, w1.y - w0.y };
            float length = Math::Length(d2);
            float radius = ScreenRadiusToWorld(camera_.get(),
                                Math::Multiply(0.5f, Math::Add(p0v, p1v)), halfThickness, 0.0f);

            groundObj_ = std::make_unique<CylinderClass>();
            groundObj_->Initialize(camera_.get(), "resources/whiteTexture.png");
            groundObj_->SetInfo(Cylinder{ center, radius, length });
            float theta  = std::atan2(d2.y, d2.x);
            float zAngle = theta - (3.14159265f * 0.5f);
            groundObj_->SetRotate(Vector3{ 0.0f, 0.0f, zAngle });

            groundRadiusWorld_ = radius;
            groundConverted_ = true;
        }

        // 右床（外側延長、内側は継ぎ目 - overlap）
        {
            Vector2 p0 = titleGroundRight_.origin;
            Vector2 p1 = titleGroundRight_.end;
            Vector2 d  = Math::Subtract(p1, p0);
            float L = Math::Length(d);
            Vector2 dir = (L > 0.0f) ? Math::Multiply(1.0f / L, d) : Vector2{1.0f, 0.0f};

            Vector2 p0v = Math::Add(p0, Math::Multiply(-innerOverlapPx, dir)); // 内側を少し左へ
            Vector2 p1v = Math::Add(p1, Math::Multiply( halfVis, dir));        // 外側を延長

            Vector3 w0 = ScreenToWorldOnZ(camera_.get(), p0v, 0.0f);
            Vector3 w1 = ScreenToWorldOnZ(camera_.get(), p1v, 0.0f);
            titleGroundWorldRight_.origin = Vector2{ w0.x, w0.y };
            titleGroundWorldRight_.end    = Vector2{ w1.x, w1.y };

            Vector3 center = Math::Multiply(0.5f, Math::Add(w0, w1));
            center.z += 0.1f;
            Vector2 d2 = Vector2{ w1.x - w0.x, w1.y - w0.y };
            float length = Math::Length(d2);
            float radius = ScreenRadiusToWorld(camera_.get(),
                                Math::Multiply(0.5f, Math::Add(p0v, p1v)), halfThickness, 0.0f);

            groundObjRight_ = std::make_unique<CylinderClass>();
            groundObjRight_->Initialize(camera_.get(), "resources/whiteTexture.png");
            groundObjRight_->SetInfo(Cylinder{ center, radius, length });
            float theta  = std::atan2(d2.y, d2.x);
            float zAngle = theta - (3.14159265f * 0.5f);
            groundObjRight_->SetRotate(Vector3{ 0.0f, 0.0f, zAngle });
        }

        // 当たり用の半径統一とカメラ初期寄せ
        if (groundObj_ && groundObjRight_) {
            float rL = groundObj_->GetInfo().radius;
            float rR = groundObjRight_->GetInfo().radius;
            groundRadiusWorld_ = std::max(rL, rR);

            Vector3 cL = groundObj_->GetInfo().center;
            Vector3 cR = groundObjRight_->GetInfo().center;
            float commonZ = std::max(cL.z, cR.z);
            cL.z = cR.z = commonZ;
            groundObj_->SetCenter(cL);
            groundObjRight_->SetCenter(cR);

            // いったん床の中点を ZoomIn のデフォルト終点に
            xEnd_ = (cL.x + cR.x) * 0.5f;

            // 初期カメラは「左床の中心」を画面 25% に寄せて、左端が映り込みにくくする
            float viewportW = camera_->GetViewportWidth();
            float viewportH = camera_->GetViewportHeight();
            Vector2 desiredScreen{ viewportW * 0.25f, viewportH * 0.5f };
            Vector3 worldAtDesired = ScreenToWorldOnZ(camera_.get(), desiredScreen, 0.0f);

            Vector3 camT = camera_->GetTranslate();
            camT.x += (cL.x - worldAtDesired.x);
            camera_->SetTranslate(camT);
            camera_->UpdateMatrix();

            xStart_ = camT.x;
            zStart_ = camT.z;
        }
    }

    // --- 文字のスクリーン→ワールド変換と初期のめり込み回避 ---
    for (auto& t : letters_) {
        Vector3 wp = ScreenToWorldOnZ(camera_.get(), t.screenPos, 0.0f);
        t.worldPos = wp;
        t.worldStartPos = wp;
        t.worldRadius = ScreenRadiusToWorld(camera_.get(), t.startScreen, t.radiusPx.x, 0.0f);
        t.worldFallSpeed = ScreenRadiusToWorld(camera_.get(), t.startScreen, t.fallSpeedPx, 0.0f);

        CollisionResult cr = isCollision(
            Vector2{ t.worldStartPos.x, t.worldStartPos.y },
            Vector2{ t.worldRadius + groundRadiusWorld_, t.worldRadius + groundRadiusWorld_ },
            titleGroundWorld_);
        if (cr.isColliding && cr.normal.y > 0.0f) {
            Vector2 push = Math::Multiply(cr.penetration + 2.0f, cr.normal);
            t.worldStartPos.x += push.x;
            t.worldStartPos.y += push.y;
            t.worldPos = t.worldStartPos;
        }

        if (t.obj) { t.obj->SetPosition(t.worldPos); }
    }

    // --- ZoomIn の最終Xを「床の中点が画面中央に来るX」に調整 ---
    if (groundObj_ && groundObjRight_) {
        // world に変換済みの床の端点を使う（長さ不一致に強くする）
        Vector2 seamL = titleGroundWorld_.end;
        Vector2 seamR = titleGroundWorldRight_.origin;
        float groundMidX = 0.5f * (seamL.x + seamR.x);

        // 深度は床オブジェクトのZを平均して使用
        float groundMidZ = 0.5f * (groundObj_->GetInfo().center.z + groundObjRight_->GetInfo().center.z);

        // ズーム後（zEnd_）に画面中心が指すワールド座標を求めるため一時的にカメラZを変更
        float viewportW = camera_->GetViewportWidth();
        float viewportH = camera_->GetViewportHeight();
        Vector2 screenCenter{ viewportW * 0.5f, viewportH * 0.5f };

        Vector3 savedCamT = camera_->GetTranslate();
        Vector3 tmpCamT = savedCamT;
        tmpCamT.z = zEnd_;
        camera_->SetTranslate(tmpCamT);
        camera_->UpdateMatrix();

        Vector3 worldAtCenter = ScreenToWorldOnZ(camera_.get(), screenCenter, groundMidZ);

        // カメラを元に戻す
        camera_->SetTranslate(savedCamT);
        camera_->UpdateMatrix();

        // groundMidX が画面中央に来るようにする
        float camXForCenteringGround = savedCamT.x + (groundMidX - worldAtCenter.x);
        xEnd_ = camXForCenteringGround;
    }
}

void TitleScene::Update() {
    // カメラの通常更新
    if (debugMode) {
        debugCamera_->Update();
        camera_->SetViewMatrix(debugCamera_->GetCamera().GetViewMatrix());
        camera_->SetPerspectiveFovMatrix(debugCamera_->GetCamera().GetPerspectiveFovMatrix());
    } else {
        camera_->Update("Camera");
    }

    // 床（描画準備）
    if (groundObj_)       groundObj_->Update("TitleGroundLeft");
    if (groundObjRight_)  groundObjRight_->Update("TitleGroundRight");

    // トランジション未開始なら通常の文字処理
    constexpr float kDeltaTime = 1.0f / 60.0f;
    if (transPhase_ == TransPhase::None) {
        // シーケンシャル落下
        fallTimer_ += kDeltaTime;
        if (!letters_.empty() && fallTimer_ >= fallInterval_) {
            fallTimer_ = 0.0f;
            letters_[fallIndex_].StartFall();
            fallIndex_ = (fallIndex_ + 1) % letters_.size();
        }
        for (auto& t : letters_) { t.UpdatePerFrameWorld(kDeltaTime, titleGroundWorld_, groundRadiusWorld_); }
    }

    // 入力で遷移開始（ワンショット）
    if (transPhase_ == TransPhase::None) {
        if (engine_->GetInputManager()->IsKeyPressed(VK_SPACE) || engine_->GetInputManager()->IsButtonPressed(XINPUT_GAMEPAD_A)) {
            se_select->Play();
            isDraw_text_press = false;
            transPhase_ = TransPhase::ZoomOut;
            transTimer_ = 0.0f;
            // 現在のXを開始値として保持（左寄りからのスタート）
            xStart_ = camera_->GetTranslate().x;
            zStart_ = -10.0f; 
            zEnd_   = -10.0f;
        }
    }

    // カメラトランジション
    if (transPhase_ == TransPhase::ZoomOut) {
        transTimer_ += kDeltaTime;
        float t = Smooth01(transTimer_ / outDuration_);
        float z = std::lerp(zStart_, zMid_, t);
        // ZoomOut中はXは固定
        camera_->SetTranslate(Vector3{ xStart_, 0.0f, z });
        camera_->UpdateMatrix();

        if (transTimer_ >= outDuration_) {
            transPhase_ = TransPhase::ZoomIn;
            transTimer_ = 0.0f;
        }
    } else if (transPhase_ == TransPhase::ZoomIn) {
        transTimer_ += kDeltaTime;
        float t = Smooth01(transTimer_ / inDuration_);
        float z = std::lerp(zMid_, zEnd_, t);
        float x = std::lerp(xStart_, xEnd_, t); // 中央へ寄せる
        camera_->SetTranslate(Vector3{ x, 0.0f, z });
        camera_->UpdateMatrix();

        if (transTimer_ > inDuration_) {

            if (g_SceneManager) {
                g_SceneManager->Request(SceneName::inGame);
            }
        }
    }

    // 3Dオブジェクト更新
    text_dan_1->Update("text_dan_1");
    text_dan_2->Update("text_dan_2");
    text_ri->Update("text_ri");
    text_sa->Update("text_sa");
    text_i->Update("text_i");
    text_ku->Update("text_ku");
    text_ru->Update("text_ru");

    text_press->Update(true, "text_press");
}

void TitleScene::Draw() {
    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Enable);
    engine_->ApplyPSO();

    // 床（黄色帯）
    if (groundObj_)       groundObj_->Draw();
    if (groundObjRight_)  groundObjRight_->Draw();

    // 文字
    text_dan_1->Draw();
    text_dan_2->Draw();
    text_ri->Draw();
    text_sa->Draw();
    text_i->Draw();
    text_ku->Draw();
    text_ru->Draw();


    engine_->SetBlend(BlendMode::kBlendModeNormal);
    engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    engine_->ApplySpritePSO();

    if (isDraw_text_press) {
        text_press->Draw();
    }
}