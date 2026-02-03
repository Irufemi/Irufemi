#include "ChaserEnemy.h"

ChaserEnemy::ChaserEnemy() : Enemy() {}

ChaserEnemy::~ChaserEnemy() {}

const char* ChaserEnemy::GetModelFile() const {
    return "TD_ChaserEnemy.obj"; // プロジェクトにモデルがなければ Enemy のデフォルトが使われる
}
