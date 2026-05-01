import sys
import io

def fix_sphere_region():
    filepath = "f:/school/3_0/CG5/CG5/project/IrufemiEngine/Renderer/Region/Primitive/SphereRegion.cpp"
    
    with io.open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()

    target = """// delete next 4 lines manually
    for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
    if (auto engine = dx_->GetEngine()) {
    }
}

void SphereRegion::SetEnvironmentCoefficient(float coefficient) {
    for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
    if (auto engine = dx_->GetEngine()) {
    }
}"""
    target2 = target.replace('\n', '\r\n')
    
    replacement = """void SphereRegion::SetEnvironmentCoefficient(float coefficient) {
    cpuMaterialData_.environmentCoefficient = coefficient;
    if (auto engine = dx_->GetEngine()) {
        for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
            engine->GetMaterialBufferManager()->Update(materialCbIndex_, cpuMaterialData_, i);
        }
    }
}"""

    if target in content:
        content = content.replace(target, replacement)
    elif target2 in content:
        content = content.replace(target2, replacement)
    else:
        print("Target not found!")
        
    with io.open(filepath, 'w', encoding='utf-8', newline='') as f:
        f.write(content)

fix_sphere_region()
