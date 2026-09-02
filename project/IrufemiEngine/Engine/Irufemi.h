#pragma once

// --- Core (数学・便利ツール) ---

#include "Core/Math/Vector2.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Vector4.h"
#include "Core/Math/Transform.h"
#include "Core/Math/Matrix3x3.h"
#include "Core/Math/Matrix4x4.h"
#include "Core/Math/Quaternion.h"
#include "Core/Math/QuaternionTransform.h"
#include "Core/Math/NumericalSequence.h"

#include "Core/Math/Geometry/AABB.h"
#include "Core/Math/Geometry/OBB.h"
#include "Core/Math/Math.h"
#include "Physics/Collision/Collision.h"
#include "Core/Math/Random/Random.h"
#include "Core/Utility/Ease.h"

#include "Core/Shape/Ball.h"
#include "Core/Shape/LinePrimitive.h"
#include "Core/Shape/Plane.h"
#include "Core/Shape/Sphere.h"
#include "Core/Shape/Triangle.h"

#include "Core/Type/BlendMode.h"

// --- Platform (OS・入力) ---
#include "Platform/Input/InputManager.h"
#include "Platform/Input/GamePad.h"
#include "Platform/Input/Keyboard.h"
#include "Platform/Input/Mouse.h"

// --- Graphics / Renderer (描画オブジェクト) ---
// LineInstanced
#include "Renderer/Object/Line/LineClass.h"
// Object2D
#include "Renderer/Object/2D/Sprite/Sprite.h"
#include "Renderer/Object/2D/Primitive/Primitive2DObject.h"
// Object3D

#include "Renderer/Object/3D/StaticModelObject/StaticModelObject.h"
#include "Renderer/Object/3D/Primitive/Primitive3DObject.h"

// ParticleGPU
#include "Renderer/System/ParticleGPU/GPUParticleSystem.h"
// VoxelParticle
#include "Renderer/System/VoxelParticle/VoxelParticleSystem.h"
// Batch
#include "Renderer/Object/Batch/ModelBatch.h"
#include "Renderer/Object/Batch/PrimitiveBatch.h"
// Skybox
#include "Renderer/Object/Skybox/Skybox.h"
// Effect
#include "Renderer/Object/Effect/Effect.h"

// 音関連
#include "Audio/AudioManager.h"
#include "Audio/AudioPlayer.h"
#include "Audio/Sound.h"

// --- Resource (素材管理) ---
#include "Resource/Texture/TextureManager.h"
#include "Resource/Model/ModelManager.h"
#include "Audio/AudioManager.h"

// --- Framework (シーン) ---
// #include "Framework/Scene/SceneManager.h"
// #include "Framework/Scene/IScene.h"

// --- Debug (デバッグ) ---
#include "Framework/UI/DebugUI.h"

// 描画
#include "Renderer/DrawManager.h"

// エンジン
#include "Core/System/IrufemiEngine.h"
