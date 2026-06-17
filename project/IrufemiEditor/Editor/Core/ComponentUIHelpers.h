#pragma once

#ifdef EditorMode
#include <memory>
#include <functional>
#include <string>
#include <type_traits>
#include "imgui/imgui.h"
#include "Framework/Component/Component.h"
#include "Framework/GameObject.h"
#include "EditorActionManager.h"
#include "EditorCommands.h"
#include "Framework/Component/Collider/SphereColliderComponent.h"

class ComponentUIHelpers {
public:
    static std::shared_ptr<Component> GetSharedComponent(GameObject* go, Component* comp);

    template <typename T>
    static void CheckUndoRedoDrag(EditorActionManager* actionManager, T* valuePtr) {
        static T startValue;
        if (ImGui::IsItemActivated()) {
            startValue = *valuePtr;
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            T endValue = *valuePtr;
            actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<T>>(
                startValue, endValue, [valuePtr](const T& v) { *valuePtr = v; }));
        }
    }

    template <typename T>
    static void CheckUndoRedoDrag(EditorActionManager* actionManager, T* valuePtr, std::function<void(const T&)> setter) {
        static T startValue;
        if (ImGui::IsItemActivated()) {
            startValue = *valuePtr;
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            T endValue = *valuePtr;
            actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<T>>(
                startValue, endValue, setter));
        }
    }

    template <typename T>
    static void PushInstantUndo(EditorActionManager* actionManager, const T& oldVal, const T& newVal, T* valuePtr) {
        actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<T>>(
            oldVal, newVal, [valuePtr](const T& v) { *valuePtr = v; }));
    }

    template <typename T>
    static void PushInstantUndo(EditorActionManager* actionManager, const T& oldVal, const T& newVal, std::function<void(const T&)> setter) {
        actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<T>>(
            oldVal, newVal, setter));
    }

    static void DrawCollisionLayerGUI(Component* comp, EditorActionManager* actionManager, uint32_t& layer, uint32_t& mask);

    template<typename T>
    static void DrawColliderCommonProperties(T* comp, EditorActionManager* actionManager) {
        Vector3 offset = comp->GetLocalOffset();
        if (ImGui::DragFloat3("Offset", &offset.x, 0.1f)) {
            comp->SetLocalOffset(offset);
        }
        CheckUndoRedoDrag(actionManager, &offset, std::function<void(const Vector3&)>([comp](const Vector3& v){ comp->SetLocalOffset(v); }));
        
        if constexpr (std::is_same_v<T, SphereColliderComponent>) {
            float radius = comp->GetLocalRadius();
            if (ImGui::DragFloat("Radius", &radius, 0.1f, 0.0f, 1000.0f)) {
                comp->SetLocalRadius(radius);
            }
            CheckUndoRedoDrag(actionManager, &radius, std::function<void(const float&)>([comp](const float& v){ comp->SetLocalRadius(v); }));
        } else {
            Vector3 size = comp->GetLocalSize();
            if (ImGui::DragFloat3("Size (Extents)", &size.x, 0.1f, 0.0f, 1000.0f)) {
                comp->SetLocalSize(size);
            }
            CheckUndoRedoDrag(actionManager, &size, std::function<void(const Vector3&)>([comp](const Vector3& v){ comp->SetLocalSize(v); }));
        }
        
        bool isTrigger = comp->isTrigger_;
        if (ImGui::Checkbox("Is Trigger", &isTrigger)) {
            PushInstantUndo(actionManager, comp->isTrigger_, isTrigger, &comp->isTrigger_);
        }
        DrawCollisionLayerGUI(comp, actionManager, comp->layer_, comp->mask_);
    }

    static void DrawFallbackPropertiesGUI(Component* component, EditorActionManager* actionManager);
};

#endif // EditorMode
