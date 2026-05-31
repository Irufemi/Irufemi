#include "FireworkEffect.h"
#include "Renderer/ParticleGPU/GPUParticleSystem.h"
#include "Engine/Graphics/Pipeline/PSOManager.h"
#include "Engine/IrufemiEngine.h"

FireworkEffect::FireworkEffect() {}

FireworkEffect::~FireworkEffect() {}

void FireworkEffect::Initialize(IrufemiEngine* engine) {
    // --- 上昇時の火の粉 (Trail) の設定 ---
    trailParticles_ = std::make_unique<GPUParticleSystem>();
    trailParticles_->Initialize("resources/circle.png");
    trailParticles_->SetBlend(BlendMode::kBlendModeAdd);
    trailParticles_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    trailParticles_->SetCull(PSOManager::CullMode::None);
    trailParticles_->SetParticleLife(0.3f, 0.5f);
    trailParticles_->SetParticleScale({0.2f, 0.2f, 0.2f}, {0.3f, 0.3f, 0.3f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f});
    trailParticles_->SetStartColor({1.0f, 0.8f, 0.2f, 1.0f}, {1.0f, 0.5f, 0.0f, 1.0f});
    trailParticles_->SetEndColor({1.0f, 0.0f, 0.0f, 0.0f}, {0.5f, 0.0f, 0.0f, 0.0f});
    trailParticles_->SetEmit(false);

    // --- 破裂 (Burst) の設定（チームメイトのパラメータを移植） ---
    burstParticles_ = std::make_unique<GPUParticleSystem>();
    burstParticles_->Initialize("resources/circle.png");
    burstParticles_->SetBlend(BlendMode::kBlendModeAdd);
    burstParticles_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    burstParticles_->SetCull(PSOManager::CullMode::None);
    // 寿命に幅を持たせ、バラバラに消えるようにする
    burstParticles_->SetParticleLife(0.7f, 0.9f);

    // 休眠による不具合を完全に回避するため、常にEmit(true)状態にしておく。
    // 代わりに frequency を 0.0f に設定することで、毎フレームの自動放出は行われず、
    // 手動での Emit(150) 呼び出し時のみバーストするようにする。
    burstParticles_->SetEmit(true);
    burstParticles_->SetSphereEmitter({0,0,0}, 0.1f, 0, 0.0f);
    // 線が強すぎないよう少し半透明の淡い光（アルファ入り）
    burstParticles_->SetStartColor({1.0f, 0.8f, 0.6f, 0.8f}, {1.0f, 1.0f, 0.9f, 1.0f});
    // 消えるときは暗いオレンジにフェードアウト
    burstParticles_->SetEndColor({0.8f, 0.2f, 0.0f, 0.0f}, {0.6f, 0.1f, 0.0f, 0.0f});
    
    // === 新しいGPUパーティクル連鎖機能を有効化 ===
    // 尾を引いて落ちる火の粉（Trail）を 0.02秒間隔で発生させる
    burstParticles_->SetEnableTrail(true, 0.02f);
    // 消滅時にパチッと弾ける閃光（Death Emit）を発生させる
    burstParticles_->SetEnableDeathEmit(true);
    
    // 新たに追加したカリング無効化メソッドを呼び出し、
    // 破裂用エミッターの初期半径が小さくても画面外判定されないようにする
    burstParticles_->SetCullingEnabled(false);
    
    // 細長い火花を表現するため、X・Zを細く、Y（進行方向）を長く設定
    // 150個だと初速15.0の拡散速度に対してスカスカになって見えなくなるため、
    // 線の長さを長めに（Y=2.0）し、存在感を出す
    burstParticles_->SetParticleScale({0.15f, 2.0f, 0.15f}, {0.2f, 2.5f, 0.2f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f});
    
    // HLSL側の仕様により、gravity は毎フレーム velocity (m/frame) から減算されるため、
    // そのまま 8.0 を渡すと 1秒後に秒速 460m で落下する隕石になってしまいます。
    // そのため、1フレームあたりの加速度になるよう 60 で割ります。
    burstParticles_->SetGravity(8.0f / 60.0f);
    // 初速が速い分、空気抵抗でシュッと止まるようにする
    burstParticles_->SetDamping(0.06f);
    burstParticles_->SetVelocityAligned(true);
}

void FireworkEffect::Fire(const Vector3& startPos, const Vector3& targetPos) {
    currentPos_ = startPos;
    targetPos_ = targetPos;
    
    // 上方向への速度ベクトルを計算（到着まで約1.2秒とする）
    float travelTime = 1.2f;
    velocity_ = { (targetPos.x - startPos.x) / travelTime, 
                  (targetPos.y - startPos.y) / travelTime, 
                  (targetPos.z - startPos.z) / travelTime };
                  
    state_ = FireworkState::Ascending;
    burstTimer_ = 0.0f;
    
    // 上昇時の火の粉は継続的に出す
    trailParticles_->SetBoxEmitter(currentPos_, {0.1f, 0.1f, 0.1f}, 2, 0.016f);
    trailParticles_->SetDirection({0.0f, -1.0f, 0.0f});
    trailParticles_->SetVelocity(0.03f); // 下へ少し勢いをつける
    trailParticles_->SetEmit(true);
}

void FireworkEffect::Update(float deltaTime) {
    if (state_ == FireworkState::Ascending) {
        currentPos_.x += velocity_.x * deltaTime;
        currentPos_.y += velocity_.y * deltaTime;
        currentPos_.z += velocity_.z * deltaTime;
        
        // 軌跡エミッターを追従させる
        trailParticles_->SetBoxEmitter(currentPos_, {0.1f, 0.1f, 0.1f}, 2, 0.016f);

        // 目標高度に到達したら破裂フェーズへ
        if (currentPos_.y >= targetPos_.y) {
            state_ = FireworkState::Bursting;
            trailParticles_->SetEmit(false); // 火の粉を止める
            burstTimer_ = 0.0f;
            
            // 破裂（バースト）
            // VelocityAligned（速度方向への引き伸ばし）が有効になったため、
            // 連続放出ではなく1回で一気に放射させ、美しい直線の軌跡を描く
            // 密度を保つため、150個から800個に増量してバーストさせる
            burstParticles_->SetSphereEmitter(currentPos_, 0.1f, 800, 0.0f);
            burstParticles_->SetSpread(1.0f); // 360度広がる
            burstParticles_->SetDirection({0.0f, 0.0f, 0.0f}); 
            // HLSL側ではvelocityは「1フレームあたりの移動量」として処理されるため、
            // IrufemiEngineのカメラ距離(Z=18)に対して秒速15mは画面外に飛ぶほど大きいため、
            // 広がりすぎないように初速を少し抑えめに調整 (15.0 -> 10.0)
            burstParticles_->SetVelocity(10.0f * deltaTime);  
            burstParticles_->Emit(800); // 800個を一気にバーストさせる
        }
    } else if (state_ == FireworkState::Bursting) {
        burstTimer_ += deltaTime;
        
        // 寿命の0.9秒 + 余裕を見て1.2秒経過で完全に終了
        if (burstTimer_ > 1.2f) {
            state_ = FireworkState::Finished;
        }
    }

    if (trailParticles_) trailParticles_->Update();
    if (burstParticles_) burstParticles_->Update();
}

void FireworkEffect::Draw() {
    if (trailParticles_) trailParticles_->Draw();
    if (burstParticles_) burstParticles_->Draw();
}
