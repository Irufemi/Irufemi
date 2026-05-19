#include "Effect.h"
#include "Renderer/ParticleGPU/GPUParticleSystem.h"
#include "Engine/Manager/DebugUI.h"
#include "Resource/Texture/TextureManager.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Irufemi.h"
#include "Renderer/Object3D/Primitive/PrimitiveObjects3DClass.h"
#include "Engine/Manager/PrimitiveManager.h"

IrufemiEngine* Effect::engine_ = nullptr;

Effect::Effect() = default;
Effect::~Effect() = default;

void Effect::Initialize(EffectType type) {
    type_ = type;
    particleSystems_.clear();
    
    switch (type_) {
    case EffectType::kHit:
    {
        isBillboard_ = true;
        
        auto system = std::make_unique<GPUParticleSystem>();
        system->Initialize(currentTextureName_);
        system->SetPrimitive(currentShape_);
        system->SetBlend(blendMode_);
        system->SetDepthWrite(depthWrite_);
        system->SetCull(cullMode_);
        system->SetBillboard(isBillboard_);
        system->SetColor(hitConfig_.color);
        system->SetParticleLife(hitConfig_.lifeMin, hitConfig_.lifeMax);
        system->SetParticleScale(
            hitConfig_.startScaleMin, hitConfig_.startScaleMax,
            hitConfig_.endScaleMin,   hitConfig_.endScaleMax
        );
        system->SetJitter(hitConfig_.jitter);
        system->SetEmit(false);
        system->SetLoop(false);
        particleSystems_.push_back(std::move(system));
        break;
    }
    case EffectType::kImpact:
    {
        isBillboard_ = false;
        
        // Planeエミッター
        auto planeSystem = std::make_unique<GPUParticleSystem>();
        planeSystem->Initialize(impactConfig_.planeTexture);
        planeSystem->SetPrimitive(impactConfig_.planeShape);
        planeSystem->SetBillboard(isBillboard_);
        planeSystem->SetBlend(blendMode_);
        planeSystem->SetDepthWrite(depthWrite_);
        planeSystem->SetCull(cullMode_);
        planeSystem->SetColor(impactConfig_.color);
        planeSystem->SetParticleLife(impactConfig_.lifeMin, impactConfig_.lifeMax);
        planeSystem->SetParticleScale(
            impactConfig_.planeStartScaleMin, impactConfig_.planeStartScaleMax,
            impactConfig_.planeEndScaleMin, impactConfig_.planeEndScaleMax
        );
        planeSystem->SetJitter(impactConfig_.jitter);
        planeSystem->SetEnableRandomRotation(impactConfig_.planeEnableRandomRotation);
        planeSystem->SetEmit(false);
        planeSystem->SetLoop(false);
        
        // Ringエミッター
        auto ringSystem = std::make_unique<GPUParticleSystem>();
        ringSystem->Initialize(impactConfig_.ringTexture);
        ringSystem->SetPrimitive(impactConfig_.ringShape);
        ringSystem->SetBillboard(isBillboard_);
        ringSystem->SetBlend(blendMode_);
        ringSystem->SetDepthWrite(depthWrite_);
        ringSystem->SetCull(cullMode_);
        ringSystem->SetColor(impactConfig_.color);
        ringSystem->SetParticleLife(impactConfig_.lifeMin, impactConfig_.lifeMax);
        ringSystem->SetParticleScale(
            impactConfig_.ringStartScaleMin, impactConfig_.ringStartScaleMax,
            impactConfig_.ringEndScaleMin, impactConfig_.ringEndScaleMax
        );
        ringSystem->SetJitter(impactConfig_.jitter);
        ringSystem->SetEnableRandomRotation(impactConfig_.ringEnableRandomRotation);
        ringSystem->SetUseClampSampler(impactConfig_.useClamp);
        ringSystem->SetEmit(false);
        ringSystem->SetLoop(false);
        
        particleSystems_.push_back(std::move(planeSystem));
        particleSystems_.push_back(std::move(ringSystem));
        break;
    }
    case EffectType::kAura:
    {
        isBillboard_ = false;
        auraObject_ = std::make_unique<PrimitiveObjects3DClass>();
        auraObject_->Initialize(PrimitiveType::Cylinder, auraConfig_.texture);
        
        // 蓋なしのCylinderリソースを取得して差し替える
        const auto& noCapCylinder = PrimitiveManager::GetInstance()->GetCylinderResource(false, false);
        if (auraObject_->GetMesh().resource) {
            auraObject_->GetMesh().resource->vertexBufferView_ = noCapCylinder.vertexBufferView;
            auraObject_->GetMesh().resource->indexBufferView_ = noCapCylinder.indexBufferView;
            auraObject_->GetMesh().resource->indexCount_ = noCapCylinder.indexCount;
        }

        auraObject_->SetCastShadows(false); // エフェクトなので影は不要
        auraObject_->GetMaterial().enableLighting = false; // ライティング不要
        auraObject_->GetMaterial().color = auraConfig_.color;
        auraObject_->GetMaterial().useClampSampler = auraConfig_.useClamp ? 3 : 0; // 3 = U:Wrap, V:Clamp
        auraObject_->SetScale(auraConfig_.scale); // 初期スケールの適用
        break;
    }
    case EffectType::kSwing:
    {
        isBillboard_ = false;
        swingObject_ = std::make_unique<PrimitiveObjects3DClass>();
        swingObject_->Initialize(PrimitiveType::Ring, swingConfig_.texture);

        // 風切りスイングに特化したカスタム形状パラメータ（端が尖った半円）
        RingParams ringParams;
        ringParams.innerRadius = swingConfig_.innerRadius;
        ringParams.startOuterRadius = 1.0f;
        ringParams.endOuterRadius = 1.0f;
        ringParams.startAngle = swingConfig_.startAngle;
        ringParams.endAngle = swingConfig_.endAngle;
        ringParams.segments = 32;
        ringParams.fadeRangeAngle = swingConfig_.fadeRangeAngle;
        ringParams.verticalUV = false;
        ringParams.innerColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        ringParams.outerColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        ringParams.startAlpha = 0.0f;
        ringParams.endAlpha = 0.0f;

        PrimitiveData ringData = PrimitiveManager::CreateRing(ringParams);
        
        // 頂点座標を XZ平面に寝かせる（ハンマーの軌道に沿ったレールにする）
        for (auto& vertex : ringData.vertices) {
            float tempY = vertex.position.y;
            vertex.position.y = 0.0f;               // Y軸を潰してXZ平面に寝かせる
            vertex.position.z = tempY;              // 元のY(上下)をZ(前後)に変換
            
            float tempNy = vertex.normal.y;
            vertex.normal.y = 0.0f;
            vertex.normal.z = tempNy;
        }

        swingObject_->ReinitializeMesh(ringData);
        
        swingObject_->SetCastShadows(false); // エフェクトなので影は不要
        swingObject_->GetMaterial().enableLighting = false; // ライティング不要
        swingObject_->GetMaterial().color = swingConfig_.color;
        swingObject_->GetMaterial().useClampSampler = swingConfig_.useClamp ? 3 : 0; // 3 = U:Wrap, V:Clamp
        swingObject_->SetScale(swingConfig_.startScale);
        isActive_ = false;
        break;
    }
    }
}

void Effect::Update() {
    float dt = GPUParticleSystem::GetEngine()->GetDeltaTime();
    if (type_ == EffectType::kImpact && particleSystems_.size() == 2) {
        currentUVOffset_.x += impactConfig_.uvScrollSpeed.x * dt;
        currentUVOffset_.y += impactConfig_.uvScrollSpeed.y * dt;
        
        Vector3 scale = { impactConfig_.uvScale.x, impactConfig_.uvScale.y, 1.0f };
        Vector3 rot = { 0.0f, 0.0f, 0.0f };
        Vector3 trans = { currentUVOffset_.x, currentUVOffset_.y, 0.0f };
        Matrix4x4 transform = Math::MakeAffineMatrix(scale, rot, trans);
        particleSystems_[1]->SetUVTransform(transform);
    } else if (type_ == EffectType::kAura && auraObject_) {
        // スクロール量の加算
        currentUVOffset_.x += auraConfig_.uvScrollSpeed.x * dt;
        currentUVOffset_.y += auraConfig_.uvScrollSpeed.y * dt;
        
        // flipVが有効な場合はスケールYを反転し、オフセットをずらす
        float scaleY = auraConfig_.flipV ? -1.0f : 1.0f;
        float offsetY = auraConfig_.flipV ? currentUVOffset_.y + 1.0f : currentUVOffset_.y;

        Vector3 scale = { 1.0f, scaleY, 1.0f };
        Vector3 rot = { 0.0f, 0.0f, 0.0f };
        Vector3 trans = { currentUVOffset_.x, offsetY, 0.0f };
        
        // 行列を構築し、MaterialComponentのuvTransformに流し込む
        auraObject_->GetMaterial().uvTransform = Math::MakeAffineMatrix(scale, rot, trans);
        auraObject_->GetMaterial().color = auraConfig_.color;
        auraObject_->GetMaterial().useClampSampler = auraConfig_.useClamp ? 3 : 0; // 3 = U:Wrap, V:Clamp
        if (auraObject_->GetMaterial().texturePath != auraConfig_.texture) {
            auraObject_->SetTexture(auraConfig_.texture);
        }
        
        auraObject_->Update();
    } else if (type_ == EffectType::kSwing && swingObject_ && isActive_) {
        lifeTimer_ -= dt;
        if (lifeTimer_ <= 0.0f) {
            isActive_ = false;
        } else {
            // 生存割合の計算 (0.0f -> 1.0f)
            float t = 1.0f - (lifeTimer_ / swingConfig_.lifeTime);
            
            // スケールを startScale から endScale へ線形補間
            Vector3 currentScale;
            currentScale.x = Lerp(swingConfig_.startScale.x, swingConfig_.endScale.x, t) * baseScale_.x;
            currentScale.y = Lerp(swingConfig_.startScale.y, swingConfig_.endScale.y, t) * baseScale_.y;
            currentScale.z = Lerp(swingConfig_.startScale.z, swingConfig_.endScale.z, t) * baseScale_.z;
            swingObject_->SetScale(currentScale);
            
            // アルファ値（透明度）をフェードアウト
            Vector4 currentColor = swingConfig_.color;
            currentColor.w = Lerp(swingConfig_.color.w, 0.0f, t);
            swingObject_->GetMaterial().color = currentColor;
            
            // 動的にメッシュを再生成し「武器の先端から弧が伸びていく」かつ「残像が消えていく」アニメーションを実装
            RingParams ringParams;
            ringParams.innerRadius = swingConfig_.innerRadius;
            ringParams.startOuterRadius = 1.0f;
            ringParams.endOuterRadius = 1.0f;
            
            // tの進行に合わせて弧を伸ばす。先端が武器を追い越さないよう、進行度tと完全に等速(1.0倍)にする
            float currentEndAngle = Lerp(0.1f, swingConfig_.endAngle, t);
            float currentStartAngle = 0.0f;
            
            // スイング後半から根本が消えていく。t=1.0の時点でも少し軌跡が残るように0.8倍で止める
            if (t > 0.5f) {
                float fadeT = std::clamp((t - 0.5f) * 2.0f, 0.0f, 1.0f);
                currentStartAngle = Lerp(0.0f, swingConfig_.endAngle * 0.8f, fadeT);
            }
            
            // 先端と根本が逆転しないように最低限の幅を確保
            if (currentEndAngle <= currentStartAngle + 1.0f) {
                currentEndAngle = currentStartAngle + 1.0f;
            }
            
            ringParams.startAngle = currentStartAngle;
            ringParams.endAngle = currentEndAngle;
            
            ringParams.segments = 32;
            ringParams.fadeRangeAngle = swingConfig_.fadeRangeAngle;
            ringParams.verticalUV = false;
            ringParams.innerColor = { 1.0f, 1.0f, 1.0f, 1.0f };
            ringParams.outerColor = { 1.0f, 1.0f, 1.0f, 1.0f };
            ringParams.startAlpha = 0.0f;
            ringParams.endAlpha = 0.0f;

            PrimitiveData ringData = PrimitiveManager::CreateRing(ringParams);
            
            // 頂点座標を XZ平面に寝かせる
            for (auto& vertex : ringData.vertices) {
                float tempY = vertex.position.y;
                vertex.position.y = 0.0f;
                vertex.position.z = tempY;
                float tempNy = vertex.normal.y;
                vertex.normal.y = 0.0f;
                vertex.normal.z = tempNy;
            }
            swingObject_->ReinitializeMesh(ringData);

            // UVスクロールの更新
            currentUVOffset_.x += swingConfig_.uvScrollSpeed.x * dt;
            currentUVOffset_.y += swingConfig_.uvScrollSpeed.y * dt;
            
            Vector3 uvScale = { swingConfig_.uvScale.x, swingConfig_.uvScale.y, 1.0f };
            Vector3 uvRot = { 0.0f, 0.0f, 0.0f };
            Vector3 uvTrans = { currentUVOffset_.x, currentUVOffset_.y, 0.0f };
            swingObject_->GetMaterial().uvTransform = Math::MakeAffineMatrix(uvScale, uvRot, uvTrans);
            swingObject_->GetMaterial().useClampSampler = swingConfig_.useClamp ? 3 : 0;
            
            if (swingObject_->GetMaterial().texturePath != swingConfig_.texture) {
                swingObject_->SetTexture(swingConfig_.texture);
            }
            
            // 最新の位置と回転を反映
            Vector3 currentRotation = baseRotation_;
            currentRotation.y -= swingConfig_.swingRotationAngle * t;
            
            swingObject_->SetPosition(basePosition_);
            swingObject_->SetRotate(currentRotation);
            swingObject_->Update();
        }
    }

    for (auto& sys : particleSystems_) {
        sys->Update();
    }
}

void Effect::SyncBeforeDraw() {
    for (auto& sys : particleSystems_) {
        sys->SyncBeforeDraw();
    }
    if (type_ == EffectType::kAura && auraObject_) {
        auraObject_->SyncBeforeDraw();
    }
    if (type_ == EffectType::kSwing && swingObject_ && isActive_) {
        swingObject_->SyncBeforeDraw();
    }
}

void Effect::Draw() {
    for (auto& sys : particleSystems_) {
        sys->Draw();
    }
    if (type_ == EffectType::kAura && auraObject_) {
        auto* engine = GPUParticleSystem::GetEngine();
        
        // 現在のステートを退避
        BlendMode prevBlend = engine->currentBlend_;
        PSOManager::DepthWrite prevDepth = engine->currentDepth_;
        PSOManager::CullMode prevCull = engine->currentCull_;

        // エフェクト用のステートを設定
        engine->SetBlend(blendMode_);
        engine->SetDepthWrite(depthWrite_);
        engine->SetCull(cullMode_);

        // 描画
        auraObject_->Draw();

        // ステートを元に戻す
        engine->SetBlend(prevBlend);
        engine->SetDepthWrite(prevDepth);
        engine->SetCull(prevCull);
    }
    if (type_ == EffectType::kSwing && swingObject_ && isActive_) {
        auto* engine = GPUParticleSystem::GetEngine();
        
        // 現在のステートを退避
        BlendMode prevBlend = engine->currentBlend_;
        PSOManager::DepthWrite prevDepth = engine->currentDepth_;
        PSOManager::CullMode prevCull = engine->currentCull_;

        // エフェクト用のステートを設定
        engine->SetBlend(blendMode_);
        engine->SetDepthWrite(depthWrite_);
        engine->SetCull(cullMode_);

        // 平面メッシュに「縦軸の厚み」を持たせるため、Y座標を少しずつズラして3枚（ミルフィーユ状）描画する
        Vector3 originalPos = basePosition_;
        
        // 1枚目（上端）
        Vector3 topPos = originalPos;
        topPos.y += 0.6f;
        swingObject_->SetPosition(topPos);
        swingObject_->Update();
        swingObject_->SyncBeforeDraw();
        swingObject_->Draw();

        // 2枚目（中央）
        swingObject_->SetPosition(originalPos);
        swingObject_->Update();
        swingObject_->SyncBeforeDraw();
        swingObject_->Draw();

        // 3枚目（下端）
        Vector3 bottomPos = originalPos;
        bottomPos.y -= 0.6f;
        swingObject_->SetPosition(bottomPos);
        swingObject_->Update();
        swingObject_->SyncBeforeDraw();
        swingObject_->Draw();
        
        // 位置を元に戻しておく
        swingObject_->SetPosition(originalPos);

        // ステートを元に戻す
        engine->SetBlend(prevBlend);
        engine->SetDepthWrite(prevDepth);
        engine->SetCull(prevCull);
    }
}

void Effect::Debug(const char* name) {
#if defined(USE_IMGUI)
    if (ImGui::Begin(name)) {
        if (ImGui::BeginTabBar("EffectParamsTabs")) {
            
            // --- 共通設定タブ ---
            if (ImGui::BeginTabItem("Common Settings")) {
                const char* typeNames[] = { "Hit", "Impact", "Aura", "Swing" };
                int currentType = static_cast<int>(type_);
                if (ImGui::Combo("Effect Type", &currentType, typeNames, IM_ARRAYSIZE(typeNames))) {
                    if (engine_) {
                        Initialize(static_cast<EffectType>(currentType));
                    }
                }
                
                ImGui::Separator();
                ImGui::Text("Pipeline Settings");
                DebugUI::DebugPsoSettings(&blendMode_, &depthWrite_, &cullMode_, "##EffectPso");
                ImGui::Checkbox("Use Billboard", &isBillboard_);

                for (auto& sys : particleSystems_) {
                    sys->SetBlend(blendMode_);
                    sys->SetDepthWrite(depthWrite_);
                    sys->SetCull(cullMode_);
                    sys->SetBillboard(isBillboard_);
                }
                
                ImGui::EndTabItem();
            }
            
            // --- 固有設定タブ ---
            if (type_ == EffectType::kHit) {
                if (ImGui::BeginTabItem("Hit Specific Config")) {
                    ImGui::Text("--- Hit Emitter Shape & Texture ---");
                    const char* primitiveShapeNames[] = { "Triangle", "Plane", "Cube", "Cylinder", "Sphere", "Tetra", "Circle", "Ring", "Skybox" };
                    int currentShape = static_cast<int>(currentShape_);
                    if (ImGui::Combo("Primitive Shape", &currentShape, primitiveShapeNames, IM_ARRAYSIZE(primitiveShapeNames))) {
                        currentShape_ = static_cast<PrimitiveType>(currentShape);
                        if (!particleSystems_.empty()) particleSystems_[0]->SetPrimitive(currentShape_);
                    }

                    if (auto* tm = GPUParticleSystem::GetTextureManager()) {
                        auto textureNames = tm->GetTextureNamesForDebug();
                        if (ImGui::BeginCombo("Texture", currentTextureName_.c_str())) {
                            for (size_t i = 0; i < textureNames.size(); i++) {
                                bool is_selected = (currentTextureName_ == textureNames[i]);
                                if (ImGui::Selectable(textureNames[i].c_str(), is_selected)) {
                                    currentTextureName_ = textureNames[i];
                                    if (!particleSystems_.empty()) particleSystems_[0]->SetTexture(currentTextureName_);
                                }
                                if (is_selected) ImGui::SetItemDefaultFocus();
                            }
                            ImGui::EndCombo();
                        }
                    }
                    
                    ImGui::Separator();
                    ImGui::Text("--- Hit Emitter Parameters ---");
                    bool changed = false;
                    if (ImGui::ColorEdit4("Color", &hitConfig_.color.x)) changed = true;
                    if (ImGui::DragFloat2("Life (Min/Max)", &hitConfig_.lifeMin, 0.01f, 0.01f, 10.0f)) changed = true;
                    if (ImGui::DragFloat("Jitter", &hitConfig_.jitter, 0.001f, 0.0f, 1.0f)) changed = true;
                    if (ImGui::DragInt("Emit Count", &hitConfig_.emitCount, 1, 1, 500)) changed = true;

                    ImGui::Separator();
                    ImGui::Text("Scale Parameters");
                    if (ImGui::DragFloat3("Start Scale Min", &hitConfig_.startScaleMin.x, 0.01f)) changed = true;
                    if (ImGui::DragFloat3("Start Scale Max", &hitConfig_.startScaleMax.x, 0.01f)) changed = true;
                    if (ImGui::DragFloat3("End Scale Min", &hitConfig_.endScaleMin.x, 0.01f)) changed = true;
                    if (ImGui::DragFloat3("End Scale Max", &hitConfig_.endScaleMax.x, 0.01f)) changed = true;

                    if (changed && !particleSystems_.empty()) {
                        particleSystems_[0]->SetColor(hitConfig_.color);
                        particleSystems_[0]->SetParticleLife(hitConfig_.lifeMin, hitConfig_.lifeMax);
                        particleSystems_[0]->SetJitter(hitConfig_.jitter);
                        particleSystems_[0]->SetParticleScale(
                            hitConfig_.startScaleMin, hitConfig_.startScaleMax,
                            hitConfig_.endScaleMin, hitConfig_.endScaleMax
                        );
                    }
                    ImGui::EndTabItem();
                }
            } 
            else if (type_ == EffectType::kImpact) {
                if (ImGui::BeginTabItem("Impact Specific Config")) {
                    bool changed = false;
                    
                    ImGui::Text("--- Plane Emitter ---");
                    const char* primitiveShapeNames[] = { "Triangle", "Plane", "Cube", "Cylinder", "Sphere", "Tetra", "Circle", "Ring", "Skybox" };
                    int currentPlaneShape = static_cast<int>(impactConfig_.planeShape);
                    if (ImGui::Combo("Plane Shape", &currentPlaneShape, primitiveShapeNames, IM_ARRAYSIZE(primitiveShapeNames))) {
                        impactConfig_.planeShape = static_cast<PrimitiveType>(currentPlaneShape);
                        if (particleSystems_.size() >= 1) particleSystems_[0]->SetPrimitive(impactConfig_.planeShape);
                    }
                    if (auto* tm = GPUParticleSystem::GetTextureManager()) {
                        auto textureNames = tm->GetTextureNamesForDebug();
                        if (ImGui::BeginCombo("Plane Texture", impactConfig_.planeTexture.c_str())) {
                            for (size_t i = 0; i < textureNames.size(); i++) {
                                bool is_selected = (impactConfig_.planeTexture == textureNames[i]);
                                if (ImGui::Selectable(textureNames[i].c_str(), is_selected)) {
                                    impactConfig_.planeTexture = textureNames[i];
                                    if (particleSystems_.size() >= 1) particleSystems_[0]->SetTexture(impactConfig_.planeTexture);
                                }
                                if (is_selected) ImGui::SetItemDefaultFocus();
                            }
                            ImGui::EndCombo();
                        }
                    }
                    
                    if (ImGui::Checkbox("Enable Random 3D Rotation##Plane", &impactConfig_.planeEnableRandomRotation)) {
                        changed = true;
                        if (particleSystems_.size() >= 1) particleSystems_[0]->SetEnableRandomRotation(impactConfig_.planeEnableRandomRotation);
                    }
                    if (ImGui::DragInt("Emit Count##Plane", &impactConfig_.planeEmitCount, 1, 1, 50)) changed = true;
                    
                    ImGui::Separator();
                    ImGui::Text("--- Ring Emitter ---");
                    int currentRingShape = static_cast<int>(impactConfig_.ringShape);
                    if (ImGui::Combo("Ring Shape", &currentRingShape, primitiveShapeNames, IM_ARRAYSIZE(primitiveShapeNames))) {
                        impactConfig_.ringShape = static_cast<PrimitiveType>(currentRingShape);
                        if (particleSystems_.size() >= 2) particleSystems_[1]->SetPrimitive(impactConfig_.ringShape);
                    }
                    if (auto* tm = GPUParticleSystem::GetTextureManager()) {
                        auto textureNames = tm->GetTextureNamesForDebug();
                        if (ImGui::BeginCombo("Ring Texture", impactConfig_.ringTexture.c_str())) {
                            for (size_t i = 0; i < textureNames.size(); i++) {
                                bool is_selected = (impactConfig_.ringTexture == textureNames[i]);
                                if (ImGui::Selectable(textureNames[i].c_str(), is_selected)) {
                                    impactConfig_.ringTexture = textureNames[i];
                                    if (particleSystems_.size() >= 2) particleSystems_[1]->SetTexture(impactConfig_.ringTexture);
                                }
                                if (is_selected) ImGui::SetItemDefaultFocus();
                            }
                            ImGui::EndCombo();
                        }
                    }
                    if (ImGui::Checkbox("Enable Random 3D Rotation##Ring", &impactConfig_.ringEnableRandomRotation)) {
                        changed = true;
                        if (particleSystems_.size() >= 2) particleSystems_[1]->SetEnableRandomRotation(impactConfig_.ringEnableRandomRotation);
                    }
                    if (ImGui::DragInt("Emit Count##Ring", &impactConfig_.ringEmitCount, 1, 1, 50)) changed = true;
                    
                    ImGui::Separator();
                    
                    ImGui::Text("--- Transform / Physics ---");
                    if (ImGui::DragFloat("Jitter (Random Walk)", &impactConfig_.jitter, 0.001f, 0.0f, 1.0f)) {
                        changed = true;
                        if (particleSystems_.size() >= 1) particleSystems_[0]->SetJitter(impactConfig_.jitter);
                        if (particleSystems_.size() >= 2) particleSystems_[1]->SetJitter(impactConfig_.jitter);
                    }

                    if (ImGui::DragFloat2("UV Scale (Ring)", &impactConfig_.uvScale.x, 0.1f)) changed = true;
                    if (ImGui::DragFloat2("UV Scroll Speed (Ring)", &impactConfig_.uvScrollSpeed.x, 0.1f)) changed = true;
                    if (ImGui::Checkbox("Use Clamp Sampler (Ring)", &impactConfig_.useClamp)) changed = true;
                    if (ImGui::ColorEdit4("Color", &impactConfig_.color.x)) changed = true;
                    if (ImGui::DragFloat2("Life (Min/Max)", &impactConfig_.lifeMin, 0.01f)) changed = true;
                    if (ImGui::DragFloat3("Plane Start Scale Min", &impactConfig_.planeStartScaleMin.x, 0.01f)) changed = true;
                    if (ImGui::DragFloat3("Plane Start Scale Max", &impactConfig_.planeStartScaleMax.x, 0.01f)) changed = true;
                    if (ImGui::DragFloat3("Plane End Scale Min", &impactConfig_.planeEndScaleMin.x, 0.01f)) changed = true;
                    if (ImGui::DragFloat3("Plane End Scale Max", &impactConfig_.planeEndScaleMax.x, 0.01f)) changed = true;
                    if (ImGui::DragFloat3("Ring Start Scale Min", &impactConfig_.ringStartScaleMin.x, 0.01f)) changed = true;
                    if (ImGui::DragFloat3("Ring Start Scale Max", &impactConfig_.ringStartScaleMax.x, 0.01f)) changed = true;
                    if (ImGui::DragFloat3("Ring End Scale Min", &impactConfig_.ringEndScaleMin.x, 0.01f)) changed = true;
                    if (ImGui::DragFloat3("Ring End Scale Max", &impactConfig_.ringEndScaleMax.x, 0.01f)) changed = true;

                    if (changed && particleSystems_.size() == 2) {
                        particleSystems_[0]->SetColor(impactConfig_.color);
                        particleSystems_[0]->SetParticleLife(impactConfig_.lifeMin, impactConfig_.lifeMax);
                        particleSystems_[0]->SetParticleScale(
                            impactConfig_.planeStartScaleMin, impactConfig_.planeStartScaleMax,
                            impactConfig_.planeEndScaleMin, impactConfig_.planeEndScaleMax
                        );
                        particleSystems_[1]->SetColor(impactConfig_.color);
                        particleSystems_[1]->SetParticleLife(impactConfig_.lifeMin, impactConfig_.lifeMax);
                        particleSystems_[1]->SetParticleScale(
                            impactConfig_.ringStartScaleMin, impactConfig_.ringStartScaleMax,
                            impactConfig_.ringEndScaleMin, impactConfig_.ringEndScaleMax
                        );
                    }
                    ImGui::EndTabItem();
                }
            } else if (type_ == EffectType::kAura) {
                if (ImGui::BeginTabItem("Aura Specific Config")) {
                    bool changed = false;
                    
                    if (auto* tm = GPUParticleSystem::GetTextureManager()) {
                        auto textureNames = tm->GetTextureNamesForDebug();
                        if (ImGui::BeginCombo("Aura Texture", auraConfig_.texture.c_str())) {
                            for (size_t i = 0; i < textureNames.size(); i++) {
                                bool is_selected = (auraConfig_.texture == textureNames[i]);
                                if (ImGui::Selectable(textureNames[i].c_str(), is_selected)) {
                                    auraConfig_.texture = textureNames[i];
                                    changed = true;
                                }
                                if (is_selected) ImGui::SetItemDefaultFocus();
                            }
                            ImGui::EndCombo();
                        }
                    }
                    
                    if (ImGui::ColorEdit4("Color", &auraConfig_.color.x)) changed = true;
                    if (ImGui::DragFloat3("Scale (Radius X/Z, Height Y)", &auraConfig_.scale.x, 0.1f)) changed = true;
                    if (ImGui::DragFloat2("UV Scroll Speed", &auraConfig_.uvScrollSpeed.x, 0.1f)) changed = true;
                    if (ImGui::Checkbox("Flip V", &auraConfig_.flipV)) changed = true;
                    if (ImGui::Checkbox("Use Clamp", &auraConfig_.useClamp)) changed = true;
                    
                    if (changed && auraObject_) {
                        auraObject_->SetScale(auraConfig_.scale);
                    }
                    
                    ImGui::EndTabItem();
                }
            } else if (type_ == EffectType::kSwing) {
                if (ImGui::BeginTabItem("Swing Specific Config")) {
                    bool changed = false;
                    
                    const char* primitiveShapeNames[] = { "Triangle", "Plane", "Cube", "Cylinder", "Sphere", "Tetra", "Circle", "Ring", "Skybox" };
                    int currentShape = static_cast<int>(swingConfig_.shape);
                    if (ImGui::Combo("Shape", &currentShape, primitiveShapeNames, IM_ARRAYSIZE(primitiveShapeNames))) {
                        swingConfig_.shape = static_cast<PrimitiveType>(currentShape);
                        changed = true;
                    }
                    
                    if (auto* tm = GPUParticleSystem::GetTextureManager()) {
                        auto textureNames = tm->GetTextureNamesForDebug();
                        if (ImGui::BeginCombo("Texture", swingConfig_.texture.c_str())) {
                            for (size_t i = 0; i < textureNames.size(); i++) {
                                bool is_selected = (swingConfig_.texture == textureNames[i]);
                                if (ImGui::Selectable(textureNames[i].c_str(), is_selected)) {
                                    swingConfig_.texture = textureNames[i];
                                    changed = true;
                                }
                                if (is_selected) ImGui::SetItemDefaultFocus();
                            }
                            ImGui::EndCombo();
                        }
                    }
                    
                    if (ImGui::ColorEdit4("Color", &swingConfig_.color.x)) changed = true;
                    if (ImGui::DragFloat3("Start Scale", &swingConfig_.startScale.x, 0.1f)) changed = true;
                    if (ImGui::DragFloat3("End Scale", &swingConfig_.endScale.x, 0.1f)) changed = true;
                    if (ImGui::DragFloat2("UV Scroll Speed", &swingConfig_.uvScrollSpeed.x, 0.1f)) changed = true;
                    if (ImGui::DragFloat2("UV Scale", &swingConfig_.uvScale.x, 0.1f)) changed = true;
                    if (ImGui::DragFloat("Life Time", &swingConfig_.lifeTime, 0.01f, 0.01f, 5.0f)) changed = true;
                    if (ImGui::Checkbox("Use Clamp", &swingConfig_.useClamp)) changed = true;
                    
                    // 新規追加したカスタムパラメータのUI
                    ImGui::Separator();
                    ImGui::Text("Ring Shape Settings");
                    if (ImGui::DragFloat("Inner Radius", &swingConfig_.innerRadius, 0.01f, 0.0f, 1.0f)) changed = true;
                    if (ImGui::DragFloat("Start Angle", &swingConfig_.startAngle, 1.0f, 0.0f, 360.0f)) changed = true;
                    if (ImGui::DragFloat("End Angle", &swingConfig_.endAngle, 1.0f, 0.0f, 360.0f)) changed = true;
                    if (ImGui::DragFloat("Fade Range Angle", &swingConfig_.fadeRangeAngle, 1.0f, 0.0f, 180.0f)) changed = true;
                    if (ImGui::DragFloat("Swing Rotation Angle (Rad)", &swingConfig_.swingRotationAngle, 0.01f, 0.0f, 6.28f)) changed = true;
                    
                    if (changed && swingObject_) {
                        Initialize(EffectType::kSwing);
                    }
                    
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }
        
        ImGui::Separator();
        if (ImGui::Button("Play Effect")) {
            Play({ 0.0f, 0.0f, 0.0f });
        }
    }
    ImGui::End();
#endif
}

void Effect::Play(const Vector3& position) {
    Play(position, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f });
}

void Effect::Play(const Vector3& position, const Vector3& rotation, const Vector3& scale) {
    basePosition_ = position;
    baseRotation_ = rotation;
    baseScale_ = scale;

    switch (type_) {
    case EffectType::kHit:
        if (!particleSystems_.empty()) {
            particleSystems_[0]->SetSphereEmitter(position, 0.0f, hitConfig_.emitCount, 0.0f);
            particleSystems_[0]->SetVelocity(0.0f); // 完全にとどまる
            particleSystems_[0]->Emit(hitConfig_.emitCount);
        }
        break;
    case EffectType::kImpact:
        if (particleSystems_.size() == 2) {
            particleSystems_[0]->SetSphereEmitter(position, 0.0f, impactConfig_.planeEmitCount, 0.0f);
            particleSystems_[0]->SetVelocity(0.0f);
            particleSystems_[0]->Emit(impactConfig_.planeEmitCount);
            
            // Zファイティングを防ぐため、Ringをほんの少し上にずらす
            Vector3 ringPos = position;
            ringPos.y += 0.001f;
            particleSystems_[1]->SetSphereEmitter(ringPos, 0.0f, impactConfig_.ringEmitCount, 0.0f);
            particleSystems_[1]->SetVelocity(0.0f);
            particleSystems_[1]->Emit(impactConfig_.ringEmitCount);
        }
        break;
    case EffectType::kAura:
        if (auraObject_) {
            auraObject_->SetPosition(position);
            auraObject_->SetScale({ auraConfig_.scale.x * scale.x, auraConfig_.scale.y * scale.y, auraConfig_.scale.z * scale.z });
            auraObject_->SetRotate(rotation);
        }
        break;
    case EffectType::kSwing:
        if (swingObject_) {
            isActive_ = true;
            lifeTimer_ = swingConfig_.lifeTime;
            currentUVOffset_ = { 0.0f, 0.0f };
            swingObject_->SetPosition(position);
            swingObject_->SetRotate(rotation);
            swingObject_->SetScale({ swingConfig_.startScale.x * scale.x, swingConfig_.startScale.y * scale.y, swingConfig_.startScale.z * scale.z });
            swingObject_->GetMaterial().color = swingConfig_.color;
        }
        break;
    }
}
