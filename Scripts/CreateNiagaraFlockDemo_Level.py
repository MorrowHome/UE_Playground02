"""Create NiagaraFlockDemo material + level. Requires NS_CubeFlock already built."""
import unreal

ROOT = "/Game/NiagaraFlockDemo"
SYS_PATH = ROOT + "/NS_CubeFlock"
MAT_PATH = ROOT + "/M_NiagaraCube"
MAP_PATH = ROOT + "/Lvl_NiagaraFlock"
BP_PATH = ROOT + "/BP_NiagaraCubeFlock"

assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.MaterialEditingLibrary


def create_material():
    if assets.does_asset_exist(MAT_PATH):
        assets.delete_asset(MAT_PATH)
    mat = tools.create_asset("M_NiagaraCube", ROOT, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("used_with_niagara_mesh_particles", True)
    color = lib.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -200, 0)
    color.set_editor_property("constant", unreal.LinearColor(0.95, 0.42, 0.12, 1.0))
    lib.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = lib.create_material_expression(mat, unreal.MaterialExpressionConstant, -200, -120)
    rough.set_editor_property("r", 0.35)
    lib.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    lib.recompile_material(mat)
    assets.save_loaded_asset(mat)
    return mat


def create_bp_wrapper(system):
    if assets.does_asset_exist(BP_PATH):
        assets.delete_asset(BP_PATH)
    # Prefer a simple NiagaraActor in the level; BP wrapper is optional.
    return None


def build_level(material):
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    if assets.does_asset_exist(MAP_PATH):
        level.load_level(MAP_PATH)
        for old in list(actors.get_all_level_actors()):
            if old.get_actor_label().startswith("NiagaraFlockDemo_"):
                actors.destroy_actor(old)
    else:
        level.new_level(MAP_PATH)

    def spawn(cls, label, location, rotation=unreal.Rotator()):
        actor = actors.spawn_actor_from_class(cls, unreal.Vector(*location), rotation)
        actor.set_actor_label("NiagaraFlockDemo_" + label)
        return actor

    ns = assets.load_asset(SYS_PATH)
    assert ns, "NS_CubeFlock missing"

    # Prefer C++ driver (mouse follow). Fall back to plain NiagaraActor.
    flock_cls = unreal.load_class(None, "/Script/CubeFlock.NiagaraFlockActor")
    if flock_cls:
        niagara_actor = spawn(flock_cls, "CubeFlock", (0, 0, 1000))
        try:
            niagara_actor.set_editor_property("flock_system", ns)
            niagara_actor.set_editor_property("b_follow_mouse", True)
            niagara_actor.set_editor_property("mouse_attraction", 2.4)
            niagara_actor.set_editor_property("flock_radius", 1400.0)
        except Exception as exc:
            unreal.log_warning(f"NiagaraFlockActor property set: {exc}")
    else:
        unreal.log_warning("NiagaraFlockActor not compiled; spawning bare NiagaraActor (no mouse follow)")
        niagara_actor = spawn(unreal.NiagaraActor, "CubeFlock", (0, 0, 1000))
        comp = niagara_actor.get_editor_property("niagara_component")
        if comp is None:
            for name in ("NiagaraComponent", "niagara_component"):
                try:
                    comp = niagara_actor.get_editor_property(name)
                    if comp:
                        break
                except Exception:
                    pass
        assert comp, "NiagaraComponent not found on NiagaraActor"
        comp.set_asset(ns)
        try:
            comp.set_editor_property("auto_activate", True)
        except Exception:
            pass

    floor = spawn(unreal.StaticMeshActor, "Floor", (0, 0, -30))
    floor.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
    floor.set_actor_scale3d(unreal.Vector(400, 400, 0.5))

    sun = spawn(unreal.DirectionalLight, "Sun", (0, 0, 2500), unreal.Rotator(-45, -35, 0))
    sun.light_component.set_editor_property("intensity", 4.0)
    sky = spawn(unreal.SkyLight, "SkyLight", (0, 0, 1800))
    sky.light_component.set_editor_property("intensity", 1.0)
    try:
        sky.light_component.set_editor_property("real_time_capture", True)
    except Exception:
        pass
    spawn(unreal.SkyAtmosphere, "Atmosphere", (0, 0, 0))

    start = unreal.Vector(2200, -2800, 2000)
    look = unreal.MathLibrary.find_look_at_rotation(start, unreal.Vector(0, 0, 1000))
    spawn(unreal.PlayerStart, "Start", (2200, -2800, 2000), look)

    gm = unreal.load_class(None, "/Script/CubeFlock.CubeFlockDemoGameMode")
    if gm:
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        world.get_world_settings().set_editor_property("default_game_mode", gm)

    unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(start, look)
    level.save_current_level()
    assets.save_asset(SYS_PATH)
    assets.save_directory(ROOT)
    unreal.log("NIAGARA_FLOCK_DEMO_CREATED " + MAP_PATH)


def main():
    assets.make_directory(ROOT)
    material = create_material()
    build_level(material)
    unreal.log("Material=" + MAT_PATH)


main()
