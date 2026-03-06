#include "Field.h"

#include "engine/Input/InputManager.h"
#include "engine/IrufemiEngine.h"
#include "camera/Camera.h"
#include "debugUI.h"

Field::Field(Camera* camera, IrufemiEngine* engine) {
	camera_ = camera;
	engine_ = engine;
}

Field::~Field() {
}

void Field::Initialize() {
	plane_ = std::make_unique<PlaneClass>();
	plane_->Initialize(camera_);
	plane_->SetScale({ 200.0f,200.0f,200.0f });
	plane_->SetRotate({ 1.57f,0.0f, 0.0f });
	plane_->SetTranslate({ 0.0f,0.0f,0.0f });
	plane_->SetColor({ 0.0f,0.5f,0.0f,1.0f });
}

void Field::Update() {
	plane_->Debug();
	plane_->Update();
}

void Field::Draw() {
	plane_->Draw();
}