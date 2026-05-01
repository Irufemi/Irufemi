import os
import glob

def replace_in_file(filepath, replacements):
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    modified = False
    for old, new in replacements:
        if old in content:
            content = content.replace(old, new)
            modified = True
            
    if modified:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f"Updated {filepath}")

def main():
    base_dir = "f:/school/3_0/CG5/CG5/project/IrufemiEngine/Renderer"
    
    # 1. BaseResource derived classes
    base_resource_replacements = [
        ("IrufemiEngine::GetInstance()", "BaseResource::GetDirectXCommon()->GetEngine()")
    ]
    
    files_to_fix_base = [
        "Particle/ParticleResource.cpp",
        "LineInstanced/LineResource.cpp",
        "Object2D/Object2DResource.cpp",
        "Object3D/Object3DResource.cpp",
        "Region/Region.cpp",
    ]
    
    for f in files_to_fix_base:
        replace_in_file(os.path.join(base_dir, f), base_resource_replacements)
        
    # 2. ObjClass
    replace_in_file(os.path.join(base_dir, "Object3D/ObjClass/ObjClass.cpp"), [
        ("IrufemiEngine::GetInstance()", "drawManager_->GetDxCommon()->GetEngine()")
    ])
    
    # 3. AnimationModel
    replace_in_file(os.path.join(base_dir, "Object3D/AnimationModel/AnimationModel.cpp"), [
        ("transformationBuffer_.Update(transformationMatrix_, frameIndex);", "engine_->GetTransformBufferManager()->Update(transformCbIndex_, transformationMatrix_);")
    ])

if __name__ == "__main__":
    main()
