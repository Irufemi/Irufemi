#pragma once
#include "3D/PlaneClass.h"
#include "3D/ObjClass.h"

class Camera;
class InputManager;
class IrufemiEngine;

class Field{
public:
	Field(Camera* camera, IrufemiEngine* engine);
	~Field();
	void Initialize();
	void Update();
	void Draw();
private:
	// 外部依存
	InputManager* input_ = nullptr;
	Camera* camera_ = nullptr;
	IrufemiEngine* engine_ = nullptr;

	// マップのモデル
	std::unique_ptr<PlaneClass> plane_ = nullptr;
	std::unique_ptr<ObjClass> map_ = nullptr;
};

