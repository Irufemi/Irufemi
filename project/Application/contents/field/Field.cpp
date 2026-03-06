#include "Field.h"

#include "engine/Input/InputManager.h"
#include "engine/IrufemiEngine.h"
#include "camera/Camera.h"

Field::Field(InputManager* input, Camera* camera, IrufemiEngine* engine) {
	input_ = input;
	camera_ = camera;
	engine_ = engine;
}

Field::~Field() {
}

void Field::Initialize() {
	plane_ = std::make_unique<PlaneClass>();
	plane_->Initialize(camera_);
}

void Field::Update() {
	plane_->Update();
}

void Field::Draw() {
	plane_->Draw();
}