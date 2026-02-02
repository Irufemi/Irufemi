#include "StripeTransition.h"
#include "engine/IrufemiEngine.h"
#include "camera/Camera.h"

void StripeTransition::Initialize(Camera* camera, IrufemiEngine* engine, Mode mode) {
    camera_ = camera;
    engine_ = engine;
    mode_ = mode;

    stripes_.clear();
    progress_.clear();

    float screenWidth = static_cast<float>(engine_->GetClientWidth());
    float screenHeight = static_cast<float>(engine_->GetClientHeight());
    float spacing = screenWidth / stripeCount_;

    for (int i = 0; i < stripeCount_; ++i) {
        auto stripe = std::make_unique<Sprite>();
        stripe->Initialize(camera_, texturePath_);
        stripe->SetSize(stripeWidth_, stripeHeight_);

        if (mode_ == Mode::In) {
            // 入り：画面外の右上から開始
            float x = (spacing + spacingOffset_) * i + 50.0f;
            float y = -screenHeight - 200.0f;
            stripe->SetPosition(x, y);
            progress_.push_back(0.0f);
        } else {
            // はけ：画面を覆っている状態から開始
            float x = (spacing + spacingOffset_) * i - 450.0f;
            float y = -50.0f;
            stripe->SetPosition(x, y);
            progress_.push_back(0.0f);
        }

        stripes_.push_back(std::move(stripe));
    }

    isActive_ = false;
    isFinished_ = false;
}

void StripeTransition::Start() {
    isActive_ = true;
    isFinished_ = false;

    for (auto& p : progress_) {
        p = 0.0f;
    }
}

void StripeTransition::Update() {
    if (!isActive_ || isFinished_) return;

    float screenWidth = static_cast<float>(engine_->GetClientWidth());
    float screenHeight = static_cast<float>(engine_->GetClientHeight());
    float spacing = screenWidth / stripeCount_;

    // トリガー位置
    float triggerY = stripeHeight_ / 2.0f;

    for (int i = 0; i < stripeCount_; ++i) {
        // 前のスプライトの位置で開始判定
        bool shouldStart = false;

        if (i == 0) {
            shouldStart = true;
        } else {
            Vector2 prevPos = stripes_[i - 1]->GetPosition2D();
            float prevTriggerPoint = prevPos.y + (stripeHeight_ * 2.0f / 3.0f);

            if (mode_ == Mode::In) {
                // 入り：上から降りてくる
                if (prevTriggerPoint >= triggerY) {
                    shouldStart = true;
                }
            } else {
                // はけ：前のスプライトが下に動き始めたら開始
                // 前のスプライトのprogressが一定以上進んだら開始
                if (progress_[i - 1] >= 0.3f) {
                    shouldStart = true;
                }
            }
        }

        // 進行度更新
        if (shouldStart && progress_[i] < 1.0f) {
            progress_[i] += moveSpeed_;
            if (progress_[i] > 1.0f) {
                progress_[i] = 1.0f;
            }
        }

        // 位置計算
        float x, y;

        if (mode_ == Mode::In) {
            // 入り：右上から左下へ
            float startX = (spacing + spacingOffset_) * i + 50.0f;
            float startY = -screenHeight - 200.0f;
            float endX = (spacing + spacingOffset_) * i - 450.0f;
            float endY = -50.0f;

            x = startX + (endX - startX) * progress_[i];
            y = startY + (endY - startY) * progress_[i];
        } else {
            // はけ：覆っている状態から左下へ
            float startX = (spacing + spacingOffset_) * i - 450.0f;
            float startY = -50.0f;
            float endX = (spacing + spacingOffset_) * i - 450.0f - 500.0f;
            float endY = -50.0f + 870.0f;

            x = startX + (endX - startX) * progress_[i];
            y = startY + (endY - startY) * progress_[i];
        }

        stripes_[i]->SetPosition(x, y);
    }

    // スプライト更新
    for (auto& stripe : stripes_) {
        stripe->Update();
    }

    // 全て完了したか判定
    bool allDone = true;
    for (const auto& p : progress_) {
        if (p < 1.0f) {
            allDone = false;
            break;
        }
    }
    if (allDone) {
        isFinished_ = true;
        isActive_ = false;
    }
}

void StripeTransition::Draw() {
    if (!isActive_ && !isFinished_ && mode_ == Mode::In) return;

    // Mode::Outは完了前のみ描画、Mode::Inは進行中のみ描画
    if (mode_ == Mode::Out && isFinished_) return;

    for (int i = stripeCount_ - 1; i >= 0; --i) {
        if (mode_ == Mode::In) {
            if (progress_[i] > 0.0f) {
                stripes_[i]->Draw();
            }
        } else {
            // はけ中：完全に消えていないものを描画
            if (progress_[i] < 1.0f || !isFinished_) {
                stripes_[i]->Draw();
            }
        }
    }
}
