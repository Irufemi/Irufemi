#pragma once
#include "Irufemi.h"

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
	std::unique_ptr<PlaneClass> pFloor_ = nullptr;
	std::unique_ptr<PlaneClass> pPZWall_ = nullptr;
	std::unique_ptr<PlaneClass> pMZWall_ = nullptr;
	std::unique_ptr<PlaneClass> pPXWall_ = nullptr;
	std::unique_ptr<PlaneClass> pMXWall_ = nullptr;
	std::unique_ptr<ObjClass> floor_ = nullptr;

	std::unique_ptr<Building> building_ = nullptr;
};

