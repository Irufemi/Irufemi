#include "VirtualEntityManagerComponent.h"
#include "Framework/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Renderer/ModelBatchRendererComponent.h"
#include "Engine/Core/Math/MathFunction.h"
#include <algorithm>

void VirtualEntityManagerComponent::Initialize() {
    batchRenderer_ = gameObject_->GetComponent<ModelBatchRendererComponent>();
    if (!batchRenderer_) {
        batchRenderer_ = gameObject_->AddComponent<ModelBatchRendererComponent>().get();
        batchRenderer_->SetUseGPUCulling(true);
    } else {
        batchRenderer_->SetUseGPUCulling(true);
    }
}

void VirtualEntityManagerComponent::Setup(int poolSize, std::function<std::shared_ptr<GameObject>()> factory) {
    maxPoolSize_ = poolSize;
    pool_ = std::make_unique<ObjectPool<GameObject>>(poolSize, factory);
}

int VirtualEntityManagerComponent::AddVirtualInstance(const Vector3& pos, const Vector3& rot, const Vector3& scale) {
    VirtualInstance vi;
    vi.id_ = nextId_++;
    vi.position_ = pos;
    vi.rotation_ = rot;
    vi.scale_ = scale;
    vi.isPromoted_ = false;
    vi.isDestroyed_ = false;
    vi.promotedInstance_ = nullptr;
    
    virtualInstances_.push_back(vi);
    return vi.id_;
}

void VirtualEntityManagerComponent::RemoveVirtualInstance(int id) {
    auto it = std::find_if(virtualInstances_.begin(), virtualInstances_.end(), [id](const VirtualInstance& vi) { return vi.id_ == id; });
    if (it != virtualInstances_.end()) {
        it->isDestroyed_ = true;
        if (it->isPromoted_ && it->promotedInstance_) {
            it->promotedInstance_->SetIsActive(false);
            if (pool_) pool_->Release(it->promotedInstance_);
            it->promotedInstance_ = nullptr;
        }
        it->isPromoted_ = false;
    }
}

std::shared_ptr<GameObject> VirtualEntityManagerComponent::Promote(int id) {
    if (!pool_) return nullptr;

    auto it = std::find_if(virtualInstances_.begin(), virtualInstances_.end(), [id](const VirtualInstance& vi) { return vi.id_ == id; });
    if (it != virtualInstances_.end() && !it->isDestroyed_ && !it->isPromoted_) {
        auto obj = pool_->Acquire();
        if (obj) {
            obj->SetIsActive(true);
            auto t = obj->GetComponent<TransformComponent>();
            if (t) {
                t->position_ = it->position_;
                t->rotation_ = it->rotation_;
                t->scale_ = it->scale_;
            }
            it->isPromoted_ = true;
            it->promotedInstance_ = obj;
            return obj;
        }
    }
    return nullptr;
}

void VirtualEntityManagerComponent::Demote(int id) {
    auto it = std::find_if(virtualInstances_.begin(), virtualInstances_.end(), [id](const VirtualInstance& vi) { return vi.id_ == id; });
    if (it != virtualInstances_.end() && it->isPromoted_ && it->promotedInstance_) {
        // 現在のTransformをVirtualに書き戻す
        auto t = it->promotedInstance_->GetComponent<TransformComponent>();
        if (t) {
            it->position_ = t->position_;
            it->rotation_ = t->rotation_;
            it->scale_ = t->scale_;
        }
        
        it->promotedInstance_->SetIsActive(false);
        if (pool_) pool_->Release(it->promotedInstance_);
        
        it->promotedInstance_ = nullptr;
        it->isPromoted_ = false;
    }
}

void VirtualEntityManagerComponent::PurgeOldestInstance() {
    // もっとも古い（リスト先頭に近い）データを消去して実体を空ける
    if (virtualInstances_.empty()) return;
    
    auto& oldVd = virtualInstances_.front();
    if (oldVd.isPromoted_ && oldVd.promotedInstance_) {
        oldVd.promotedInstance_->SetIsActive(false);
        if (pool_) pool_->Release(oldVd.promotedInstance_);
    }
    virtualInstances_.erase(virtualInstances_.begin());
}

void VirtualEntityManagerComponent::ReleaseGameObject(std::shared_ptr<GameObject> obj) {
    if (obj) {
        obj->SetIsActive(false);
        if (pool_) pool_->Release(obj);
    }
}

void VirtualEntityManagerComponent::Update() {
    if (!batchRenderer_) return;
    
    batchRenderer_->ClearInstances();
    
    // 仮想インスタンス（未昇格）の描画
    for (const auto& vi : virtualInstances_) {
        if (!vi.isDestroyed_ && !vi.isPromoted_) {
            Matrix4x4 mat = Math::MakeAffineMatrix(vi.scale_, vi.rotation_, vi.position_);
            batchRenderer_->AddInstanceWorld(mat);
        }
    }
    
    // 実体化済みのインスタンスの描画
    // 子供のオブジェクト群（プールから取得されてアクティブなもの）を走査
    for (const auto& child : gameObject_->GetChildren()) {
        if (child && child->GetIsActive() && !child->IsDestroyed()) {
            auto t = child->GetComponent<TransformComponent>();
            if (t) {
                batchRenderer_->AddInstanceWorld(t->GetWorldMatrix());
            }
        }
    }
}
