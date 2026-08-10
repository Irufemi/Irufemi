#pragma once

#ifdef EditorMode
#include "../Core/IComponentEditor.h"

class AABBColliderComponentEditor : public IComponentEditor {
public:
    void Draw(Component* component, EditorActionManager* actionManager) override;
};

class OBBColliderComponentEditor : public IComponentEditor {
public:
    void Draw(Component* component, EditorActionManager* actionManager) override;
};

class SphereColliderComponentEditor : public IComponentEditor {
public:
    void Draw(Component* component, EditorActionManager* actionManager) override;
};

#endif // EditorMode
