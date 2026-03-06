#include "Field.h"

#include "engine/Input/InputManager.h"
#include "engine/IrufemiEngine.h"
#include "camera/Camera.h"
#include "debugUI.h"

Field::Field(Camera* camera, IrufemiEngine* engine) {
	camera_ = camera;
	engine_ = engine;
	input_ = engine_->GetInputManager();
}

Field::~Field() {
}

void Field::Initialize() {
	pFloor_ = std::make_unique<PlaneClass>();
	pFloor_->Initialize(camera_);
	pFloor_->SetScale({ 200.0f,200.0f,200.0f });
	pFloor_->SetRotate({ 1.57f,0.0f, 0.0f });
	pFloor_->SetTranslate({ 0.0f,0.0f,0.0f });
	pFloor_->SetColor({ 0.0f,1.0f,0.0f,1.0f });

	pPZWall_ = std::make_unique<PlaneClass>();
	pPZWall_->Initialize(camera_);
	pPZWall_->SetScale({ 200.0f,10.0f,200.0f });
	pPZWall_->SetTranslate({ 0.0f,5.0f,100.0f });
	pPZWall_->SetColor({ 0.0f,0.0f,1.0f,1.0f });

	pMZWall_ = std::make_unique<PlaneClass>();
	pMZWall_->Initialize(camera_);
	pMZWall_->SetScale({ 200.0f,10.0f,200.0f });
	pMZWall_->SetRotate({ 0.0f,3.14f, 0.0f });
	pMZWall_->SetTranslate({ 0.0f,5.0f,-100.0f });
	pMZWall_->SetColor({ 0.0f,0.0f,1.0f,1.0f });

	pPXWall_ = std::make_unique<PlaneClass>();
	pPXWall_->Initialize(camera_);
	pPXWall_->SetScale({ 200.0f,10.0f,200.0f });
	pPXWall_->SetRotate({ 0.0f,1.57f, 0.0f });
	pPXWall_->SetTranslate({ 100.0f,5.0f,0.0f });
	pPXWall_->SetColor({ 0.0f,0.0f,1.0f,1.0f });

	pMXWall_ = std::make_unique<PlaneClass>();
	pMXWall_->Initialize(camera_);
	pMXWall_->SetScale({ 200.0f,10.0f,200.0f });
	pMXWall_->SetRotate({ 0.0f,-1.57f, 0.0f });
	pMXWall_->SetTranslate({ -100.0f,5.0f,0.0f });
	pMXWall_->SetColor({ 0.0f,0.0f,1.0f,1.0f });
}

void Field::Update() {
	pFloor_->Debug();
	pFloor_->Update();
	pPZWall_->Update();
	pMZWall_->Update();
	pPXWall_->Update();
	pMXWall_->Update();
}

void Field::Draw() {
	pFloor_->Draw();
	pPZWall_->Draw();
	pMZWall_->Draw();
	pPXWall_->Draw();
	pMXWall_->Draw();
}