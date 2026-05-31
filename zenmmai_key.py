"""
ぜんまいキー - 完全修正版
画像通り：薄い横長板（丸穴×2）+ 円筒グリップ のシンプルな形
"""

import bpy
import math

# ============================================================
# シーンクリア
# ============================================================
def clear_scene():
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete()
    for d in bpy.data.meshes:
        bpy.data.meshes.remove(d)
    for d in bpy.data.materials:
        bpy.data.materials.remove(d)

# ============================================================
# ブーリアン差分
# ============================================================
def bool_cut(target, cutter):
    bpy.context.view_layer.objects.active = target
    mod = target.modifiers.new("Cut", 'BOOLEAN')
    mod.operation = 'DIFFERENCE'
    mod.object     = cutter
    mod.solver     = 'EXACT'
    bpy.ops.object.modifier_apply(modifier=mod.name)
    bpy.data.objects.remove(cutter, do_unlink=True)

# ============================================================
# マテリアル（シルバーメタル）
# ============================================================
def make_silver():
    mat = bpy.data.materials.new("Silver")
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()
    bsdf = nodes.new('ShaderNodeBsdfPrincipled')
    bsdf.inputs['Base Color'].default_value    = (0.72, 0.75, 0.78, 1.0)
    bsdf.inputs['Metallic'].default_value      = 0.95
    bsdf.inputs['Roughness'].default_value     = 0.20
    out = nodes.new('ShaderNodeOutputMaterial')
    out.location = (300, 0)
    links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])
    return mat

# ============================================================
# 1. キーヘッド
#    横幅 2.8 / 高さ 1.4 / 厚み 0.22
#    穴 radius 0.38 × 2個（左右）
#    上下マージン: 0.70 - 0.38 = 0.32  ← 十分な幅
#    左右マージン: 1.40 - (0.65+0.38) = 0.37  ← 十分な幅
#    中央ブリッジ: 0.65 - 0.38 = 0.27  ← キーらしい細さ
# ============================================================
def create_head(mat):
    # 板本体
    bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 0, 0))
    head = bpy.context.active_object
    head.name = "KeyHead"
    # scale: (X半幅=1.40, Y半厚=0.11, Z半高=0.70)
    head.scale = (1.40, 0.11, 0.70)
    bpy.ops.object.transform_apply(scale=True)

    # 左穴カッター（Y軸方向に貫通）
    bpy.ops.mesh.primitive_cylinder_add(
        radius=0.38, depth=0.50, vertices=64,
        location=(-0.65, 0, 0)
    )
    cut_l = bpy.context.active_object
    cut_l.rotation_euler = (math.radians(90), 0, 0)
    bpy.ops.object.transform_apply(rotation=True)

    # 右穴カッター
    bpy.ops.mesh.primitive_cylinder_add(
        radius=0.38, depth=0.50, vertices=64,
        location=(0.65, 0, 0)
    )
    cut_r = bpy.context.active_object
    cut_r.rotation_euler = (math.radians(90), 0, 0)
    bpy.ops.object.transform_apply(rotation=True)

    bool_cut(head, cut_l)
    bool_cut(head, cut_r)

    # 軽くベベル（角を落とす）
    bev = head.modifiers.new("Bevel", 'BEVEL')
    bev.width    = 0.04
    bev.segments = 4
    bpy.context.view_layer.objects.active = head
    bpy.ops.object.modifier_apply(modifier=bev.name)
    bpy.ops.object.shade_smooth()

    head.data.materials.append(mat)
    return head

# ============================================================
# 2. グリップ（円筒）
#    radius 0.30 / 高さ 1.20
#    ヘッド下端 Z = -0.70 から続く
# ============================================================
def create_grip(mat):
    grip_center_z = -0.70 - 0.60  # ヘッド下端 - グリップ半高
    bpy.ops.mesh.primitive_cylinder_add(
        radius=0.30, depth=1.20, vertices=64,
        location=(0, 0, grip_center_z)
    )
    grip = bpy.context.active_object
    grip.name = "Grip"

    bev = grip.modifiers.new("Bevel", 'BEVEL')
    bev.width         = 0.025
    bev.segments      = 4
    bev.limit_method  = 'ANGLE'
    bev.angle_limit   = math.radians(60)
    bpy.ops.object.modifier_apply(modifier=bev.name)
    bpy.ops.object.shade_smooth()

    grip.data.materials.append(mat)
    return grip

# ============================================================
# メイン
# ============================================================
def main():
    clear_scene()
    mat = make_silver()

    head = create_head(mat)
    grip = create_grip(mat)

    # コレクション
    col = bpy.data.collections.new("ZenmaiKey")
    bpy.context.scene.collection.children.link(col)
    for obj in [head, grip]:
        bpy.context.scene.collection.objects.unlink(obj)
        col.objects.link(obj)

    # ----------------------------------------------------------
    # カメラ（正面やや斜め、上から少し俯瞰）
    # ----------------------------------------------------------
    bpy.ops.object.camera_add(location=(0, -5.0, 0.0))
    cam = bpy.context.active_object
    cam.name = "Camera"
    cam.rotation_euler = (math.radians(90), 0, 0)
    cam.data.lens = 85
    bpy.context.scene.camera = cam

    # ----------------------------------------------------------
    # ライト
    # ----------------------------------------------------------
    bpy.ops.object.light_add(type='AREA', location=(3, -2, 5))
    l1 = bpy.context.active_object
    l1.data.energy = 600
    l1.data.size   = 4.0
    l1.rotation_euler = (math.radians(40), 0, math.radians(20))

    bpy.ops.object.light_add(type='AREA', location=(-4, -1, 2))
    l2 = bpy.context.active_object
    l2.data.energy = 200
    l2.data.size   = 5.0

    bpy.ops.object.light_add(type='AREA', location=(0, 4, 3))
    l3 = bpy.context.active_object
    l3.data.energy = 300
    l3.data.color  = (0.85, 0.92, 1.0)
    l3.data.size   = 3.0

    # ワールド
    world = bpy.context.scene.world
    if not world:
        world = bpy.data.worlds.new("World")
        bpy.context.scene.world = world
    world.use_nodes = True
    bg = world.node_tree.nodes.get('Background')
    if bg:
        bg.inputs['Color'].default_value    = (0.12, 0.12, 0.14, 1.0)
        bg.inputs['Strength'].default_value = 0.3

    # レンダー設定
    sc = bpy.context.scene
    sc.render.engine     = 'CYCLES'
    sc.cycles.samples    = 256
    sc.render.resolution_x = 1080
    sc.render.resolution_y = 1350

    print("完了: KeyHead + Grip の2パーツ")

main()
