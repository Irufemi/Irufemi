#include "GameObject.h"

void GameObject::Initialize() {
    for (auto& comp : components_) {
        comp->Initialize();
    }
}

void GameObject::Update() {
    if (!isActive_) return;
    for (auto& comp : components_) {
        comp->Update();
    }
}

void GameObject::Draw() {
    if (!isActive_) return;
    for (auto& comp : components_) {
        comp->Draw();
    }
}

void GameObject::OnInspectorGUI() {
#ifdef EditorMode
    for (auto& comp : components_) {
        if (comp) comp->OnInspectorGUI();
    }
#endif
}
