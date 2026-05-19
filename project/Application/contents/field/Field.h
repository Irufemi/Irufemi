#pragma once
#include "Irufemi.h"
#include "Renderer/Object3D/Primitive/PrimitiveObjects3DClass.h"

class Camera;
class InputManager;
class IrufemiEngine;
class Building;

class Field{
public:
	Field(IrufemiEngine* engine);
	~Field();
	void Initialize(IrufemiEngine* engine) { engine_ = engine; Initialize(); }
	void Initialize();
	void Update();
	void Draw();

	Building* GetBuilding() const { return building_.get(); }

private:
	// 外部依存
	InputManager* input_ = nullptr;

	IrufemiEngine* engine_ = nullptr;

	// マップのモデル
	std::unique_ptr<PrimitiveObjects3DClass> pFloor_ = nullptr;
	std::unique_ptr<PrimitiveObjects3DClass> pPZWall_ = nullptr;
	std::unique_ptr<PrimitiveObjects3DClass> pMZWall_ = nullptr;
	std::unique_ptr<PrimitiveObjects3DClass> pPXWall_ = nullptr;
	std::unique_ptr<PrimitiveObjects3DClass> pMXWall_ = nullptr;
	std::unique_ptr<ObjClass> floor_ = nullptr;

	std::unique_ptr<Building> building_ = nullptr;
};

