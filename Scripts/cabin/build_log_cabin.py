"""Rebuild the hermit's carpentry in the saved trail map. Run in Unreal Editor."""

from pathlib import Path
import sys
import unreal as u

sys.path.insert(0, str(Path(__file__).resolve().parent))
from carpentry_mesh import shell, door

DEST = "/Game/NightOfTheWolves/FirstLeg/Carpentry"
SRC = (
    Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
    / "Art/Source/FirstLeg/Carpentry"
)
assets = u.EditorAssetLibrary
tools = u.AssetToolsHelpers.get_asset_tools()
edit = u.MaterialEditingLibrary
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
levels = u.get_editor_subsystem(u.LevelEditorSubsystem)


def load_maps():
    tasks = []
    for source in sorted(SRC.glob("*.jpg")):
        if not assets.does_asset_exist(f"{DEST}/{source.stem}"):
            task = u.AssetImportTask()
            task.set_editor_properties(
                dict(
                    filename=str(source),
                    destination_path=DEST,
                    automated=True,
                    save=True,
                )
            )
            tasks.append(task)
    if tasks:
        tools.import_asset_tasks(tasks)
    maps = {}
    for name in ("Hewn", "Sawn"):
        for ch in ("Color", "Normal", "Roughness"):
            key = name + ch
            tex = assets.load_asset(f"{DEST}/T_{key}")
            if tex is None:
                raise RuntimeError("Missing scan " + key)
            if ch != "Color":
                tex.set_editor_property("srgb", False)
                tex.set_editor_property(
                    "compression_settings",
                    u.TextureCompressionSettings.TC_NORMALMAP
                    if ch == "Normal"
                    else u.TextureCompressionSettings.TC_MASKS,
                )
                assets.save_loaded_asset(tex)
            maps[key] = tex
    return maps


def surface(name, maps, scan=None, tint=(1, 1, 1), metal=0):
    path = DEST + "/" + name
    if assets.does_asset_exist(path):
        m = assets.load_asset(path)
        edit.delete_all_material_expressions(m)
    else:
        m = tools.create_asset(name, DEST, u.Material, u.MaterialFactoryNew())

    def node(kind, x, y, **kwargs):
        n = edit.create_material_expression(m, kind, x, y)
        if kwargs:
            n.set_editor_properties(kwargs)
        return n

    def link(a, out, b, pin):
        if not edit.connect_material_expressions(a, out, b, pin):
            raise RuntimeError("Missing material input " + pin)

    def prop(a, out, which):
        edit.connect_material_property(a, out, which)

    def scalar(v, y):
        return node(u.MaterialExpressionConstant, -260, y, r=v)

    color = node(
        u.MaterialExpressionConstant3Vector, -700, 0, constant=u.LinearColor(*tint, 1)
    )
    if scan:
        src = node(
            u.MaterialExpressionTextureSample, -1040, -210, texture=maps[scan + "Color"]
        )
        mul = node(u.MaterialExpressionMultiply, -470, -60)
        link(src, "RGB", mul, "A")
        link(color, "", mul, "B")
        color = mul
        normal = node(
            u.MaterialExpressionTextureSample,
            -620,
            250,
            texture=maps[scan + "Normal"],
            sampler_type=u.MaterialSamplerType.SAMPLERTYPE_NORMAL,
        )
        prop(normal, "RGB", u.MaterialProperty.MP_NORMAL)
        rough = node(
            u.MaterialExpressionTextureSample,
            -620,
            430,
            texture=maps[scan + "Roughness"],
            sampler_type=u.MaterialSamplerType.SAMPLERTYPE_MASKS,
        )
        prop(rough, "R", u.MaterialProperty.MP_ROUGHNESS)
    elif name == "M_CutEnds":
        uv = node(u.MaterialExpressionTextureCoordinate, -910, -280)
        ring = node(u.MaterialExpressionCustom, -530, -250)
        inp = u.CustomInput()
        inp.set_editor_property("input_name", "UV")
        ring.set_editor_properties(
            dict(
                inputs=[inp],
                output_type=u.CustomMaterialOutputType.CMOT_FLOAT1,
                description="Uneven growth rings and seasoning checks on each end",
                code="""
float2 p=UV*2-1+float2(.046,-.065);
float a=atan2(p.y,p.x), r=length(p);
float d=r*(1+.035*sin(a*3)+.025*cos(a*5))+.018*sin(13*r+2*sin(a));
float rings=1-smoothstep(.07,.30,abs(frac(d*24+.5*sin(d*22))-.50));
float split=1-smoothstep(.006,.022,abs(sin(a*2.5+.055*sin(r*14)+1.1)));
float rays=sin(a*119+d*10);
return (.86-.33*rings-.41*split*smoothstep(.31,.87,r)+.028*rays) * lerp(1,.69,smoothstep(.83,.97,r));
""",
            )
        )
        link(uv, "", ring, "UV")
        mul = node(u.MaterialExpressionMultiply, -300, -70)
        link(color, "", mul, "A")
        link(ring, "", mul, "B")
        color = mul
    if not scan:
        prop(scalar(0.56 if metal else 0.91, 430), "", u.MaterialProperty.MP_ROUGHNESS)
    vertex = node(u.MaterialExpressionVertexColor, -430, 110)
    varied = node(u.MaterialExpressionMultiply, -80, 40)
    link(color, "", varied, "A")
    link(vertex, "", varied, "B")
    prop(varied, "", u.MaterialProperty.MP_BASE_COLOR)
    if metal:
        prop(scalar(metal, 550), "", u.MaterialProperty.MP_METALLIC)
    assets.save_loaded_asset(m, False)
    return m


def bake(name, source, palette, folder=DEST):
    mesh = source.mesh()
    path = folder + "/" + name
    target = assets.load_asset(path) if assets.does_asset_exist(path) else None
    if target is None:
        opts = u.GeometryScriptCreateNewStaticMeshAssetOptions()
        opts.set_editor_properties(
            dict(enable_collision=False, enable_recompute_tangents=True)
        )
        target, pin = (
            u.GeometryScript_NewAssetUtils.create_new_static_mesh_asset_from_mesh(
                mesh, path, opts
            )
        )
        if pin != u.GeometryScriptOutcomePins.SUCCESS:
            raise RuntimeError("Could not create " + path)
    opts = u.GeometryScriptCopyMeshToAssetOptions()
    opts.set_editor_properties(
        dict(
            enable_recompute_tangents=True,
            replace_materials=True,
            new_materials=palette,
        )
    )
    _, pin = u.GeometryScript_AssetUtils.copy_mesh_to_static_mesh(
        mesh, target, opts, u.GeometryScriptMeshWriteLOD()
    )
    if pin != u.GeometryScriptOutcomePins.SUCCESS:
        raise RuntimeError("Could not update " + path)
    if not assets.save_loaded_asset(target, False):
        raise RuntimeError("Could not save " + path)
    u.log(
        f"CARPENTRY BAKED {name}: {mesh.get_triangle_count()} triangles, {len(target.static_materials)} materials"
    )
    return target


def install():
    if not levels.load_level("/Game/NightOfTheWolves/Maps/M1_Trail"):
        raise RuntimeError("Could not open trail")
    by = {a.get_actor_label(): a for a in actors.get_all_level_actors()}
    blockers = (
        "Cabin front left",
        "Cabin front right",
        "Cabin lintel",
        "Cabin back",
        "Cabin left",
        "Cabin right",
        "Cabin roof",
        "M5 window wall right",
        "M5 window sill wall",
        "M5 window lintel wall",
        "Cabin door",
        "Cabin front left header",
        "Cabin front right header",
        "Cabin entrance ceiling",
        "Cabin ceiling left",
        "Cabin ceiling right",
    )
    if not all(label in by for label in blockers):
        raise RuntimeError("Unexpected cabin layout")
    panel = by["Cabin door"]
    if u.Name("M2Door") not in panel.tags:
        raise RuntimeError("Missing moving panel")
    scans = load_maps()
    palette = [
        surface("M_HewnLogs", scans, "Hewn", (0.86, 0.69, 0.53)),
        surface("M_AdzedBoards", scans, "Sawn", (0.83, 0.72, 0.59)),
        surface("M_CutEnds", scans, tint=(0.33, 0.235, 0.14)),
        surface("M_ClayChinking", scans, tint=(0.11, 0.09, 0.065)),
        surface("M_ForgedStraps", scans, tint=(0.080, 0.075, 0.063), metal=0.72),
        surface("M_CedarShakes", scans, "Sawn", (0.48, 0.46, 0.40)),
    ]
    walls, leaf = (
        bake("SM_NotchedLogHouse", shell(), palette),
        bake("SM_LedgedDoor", door(), palette),
    )

    def place(label, asset, location, movable=False):
        a = by.get(label) or actors.spawn_actor_from_class(u.StaticMeshActor, location)
        a.modify()
        a.set_actor_label(label)
        c = a.static_mesh_component
        c.set_mobility(
            u.ComponentMobility.MOVABLE if movable else u.ComponentMobility.STATIC
        )
        c.set_static_mesh(asset)
        c.set_collision_profile_name("NoCollision")
        a.set_actor_location(location, False, False)
        return a

    for label in (*blockers, "Shovel placeholder handle", "Shovel placeholder blade"):
        a = by.get(label)
        if a:
            a.modify()
            a.static_mesh_component.modify()
            a.static_mesh_component.set_visibility(False)
            a.static_mesh_component.set_editor_property("cast_shadow", False)
    place("Hermit cabin - saddle-notched timbers", walls, u.Vector(2850, 0, 0))
    moving = place("Hermit cabin - ledged door", leaf, panel.get_actor_location(), True)
    keep = u.AttachmentRule.KEEP_WORLD
    moving.attach_to_actor(panel, "", keep, keep, keep, False)
    if not levels.save_current_level():
        raise RuntimeError("Could not save completed cabin")
    u.log(
        "CARPENTRY SAVED M1_Trail; original wall blockers and moving doorway preserved"
    )


if __name__ == "__main__":
    install()
