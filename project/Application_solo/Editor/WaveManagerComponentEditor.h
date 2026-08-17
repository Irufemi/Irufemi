#pragma once

#if defined(_DEBUG) || defined(EditorMode) || defined(DEVELOPMENT)
#include "Core/IComponentEditor.h"

class WaveManagerComponentEditor : public IComponentEditor {
public:
    WaveManagerComponentEditor() = default;
    ~WaveManagerComponentEditor() override = default;

    void Draw(Component* component, class EditorActionManager* actionManager) override;
};
#endif
