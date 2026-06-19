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
    }
}

void VirtualEntityManagerComponent::Setup(int poolSize, int maxVirtualInstances, std::function<std::shared_ptr<GameObject>()> factory) {
    maxPoolSize_ = poolSize;
    maxVirtualInstances_ = maxVirtualInstances;
    pool_ = std::make_unique<ObjectPool<GameObject>>(poolSize, factory);

    dense_.reserve(maxVirtualInstances_);
    sparse_.resize(maxVirtualInstances_, -1);
    
    // IDを再利用可能なキューに積む
    std::queue<int> empty;
    std::swap(freeIds_, empty); // キューをリセット
    for (int i = 0; i < maxVirtualInstances_; ++i) {
        freeIds_.push(i);
    }
}

int VirtualEntityManagerComponent::AddVirtualInstance(const Vector3& pos, const Vector3& rot, const Vector3& scale) {
    if (freeIds_.empty()) return -1; // 上限到達

    int id = freeIds_.front();
    freeIds_.pop();

    VirtualInstance vi;
    vi.id_ = id;
    vi.position_ = pos;
    vi.rotation_ = rot;
    vi.scale_ = scale;
    vi.isPromoted_ = false;
    vi.isDestroyed_ = false;
    vi.promotedInstance_ = nullptr;
    
    dense_.push_back(vi);
    sparse_[id] = static_cast<int>(dense_.size() - 1);

    return id;
}

void VirtualEntityManagerComponent::RemoveVirtualInstance(int id) {
    if (id < 0 || id >= maxVirtualInstances_) return;
    int denseIndex = sparse_[id];
    if (denseIndex == -1) return; // すでに存在しない

    auto& vi = dense_[denseIndex];
    if (vi.isPromoted_ && vi.promotedInstance_) {
        vi.promotedInstance_->SetIsActive(false);
        if (pool_) pool_->Release(vi.promotedInstance_);
        vi.promotedInstance_ = nullptr;
    }

    // Sparse Setの実装：削除対象と末尾要素をスワップして削除（O(1)）
    int lastDenseIndex = static_cast<int>(dense_.size() - 1);
    if (denseIndex != lastDenseIndex) {
        // 末尾の要素を削除対象の位置に移動
        dense_[denseIndex] = dense_[lastDenseIndex];
        // 移動した要素のsparse_を更新
        sparse_[dense_[denseIndex].id_] = denseIndex;
    }
    
    dense_.pop_back();
    sparse_[id] = -1; // 削除済みマーク
    freeIds_.push(id); // IDを解放して再利用可能にする
}

std::shared_ptr<GameObject> VirtualEntityManagerComponent::Promote(int id) {
    if (!pool_) return nullptr;
    if (id < 0 || id >= maxVirtualInstances_) return nullptr;
    
    int denseIndex = sparse_[id];
    if (denseIndex == -1) return nullptr;

    auto& vi = dense_[denseIndex];
    if (!vi.isDestroyed_ && !vi.isPromoted_) {
        auto obj = pool_->Acquire();
        if (obj) {
            obj->SetIsActive(true);
            auto t = obj->GetComponent<TransformComponent>();
            if (t) {
                t->SetPosition(vi.position_);
                t->SetRotation(vi.rotation_);
                t->SetScale(vi.scale_);
            }
            vi.isPromoted_ = true;
            vi.promotedInstance_ = obj;
            return obj;
        }
    }
    return nullptr;
}

void VirtualEntityManagerComponent::Demote(int id) {
    if (id < 0 || id >= maxVirtualInstances_) return;
    
    int denseIndex = sparse_[id];
    if (denseIndex == -1) return;

    auto& vi = dense_[denseIndex];
    if (vi.isPromoted_ && vi.promotedInstance_) {
        // 現在のTransformをVirtualに書き戻す
        auto t = vi.promotedInstance_->GetComponent<TransformComponent>();
        if (t) {
            vi.position_ = t->GetPosition();
            vi.rotation_ = t->GetRotation();
            vi.scale_ = t->GetScale();
        }
        
        vi.promotedInstance_->SetIsActive(false);
        if (pool_) pool_->Release(vi.promotedInstance_);
        
        vi.promotedInstance_ = nullptr;
        vi.isPromoted_ = false;
    }
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
    for (auto& vi : dense_) {
        if (!vi.isPromoted_) {
            Transform t;
            t.translate = vi.position_;
            t.rotate = vi.rotation_;
            t.scale = vi.scale_;
            batchRenderer_->AddInstance(t);
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
