#pragma once

#ifdef EditorMode
#include "IrufemiEditor/Core/IComponentEditor.h"

class WaveManagerComponentEditor : public IComponentEditor {
public:
    WaveManagerComponentEditor() = default;
    ~WaveManagerComponentEditor() override = default;

    void Draw(Component* component, class EditorActionManager* actionManager) override;
};
#endif
