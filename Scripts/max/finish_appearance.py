"""Build Max's game skin materials and fit the Creator Side Swept Fringe cards to his head socket.

Run with Unreal Editor Python after create_max.py. MAX_APPEARANCE=hair rebuilds only the cards.
"""
import math
import os
import unreal as u

ROOT = "/Game/NightOfTheWolves/Max/Anatomy"
PLUGIN = "/MetaHumanCharacter"
GROOMS = PLUGIN + "/Optional/Grooms/GroomAssets"
HAIR = GROOMS + "/Hair/Hair_S_SideSweptFringe/Hair_S_SideSweptFringe"
assets = u.EditorAssetLibrary
edit = u.MaterialEditingLibrary
tools = u.AssetToolsHelpers.get_asset_tools()


def load(path):
    obj = u.load_asset(path)
    if not obj:
        raise RuntimeError("Missing " + path)
    return obj


def store(obj):
    if not assets.save_loaded_asset(obj, False):
        raise RuntimeError("Failed to save " + obj.get_path_name())


class Surface:
    def __init__(self, name):
        path = ROOT + "/Materials/" + name
        self.mat = load(path) if assets.does_asset_exist(path) else tools.create_asset(
            name, ROOT + "/Materials", u.Material, u.MaterialFactoryNew())
        # The playable Blueprint may already hold these expressions during editor
        # startup. Update the authored nodes in place on subsequent runs.
        self.existing = {(type(node), *edit.get_material_expression_node_position(node)): node
                         for node in edit.get_material_expressions(self.mat)}

    def node(self, cls, x, y, **props):
        obj = self.existing.get((cls, x, y)) or edit.create_material_expression(self.mat, cls, x, y)
        if isinstance(obj, u.MaterialExpressionCustom) and "inputs" in props:
            pin_names = lambda pins: [str(pin.get_editor_property("input_name")) for pin in pins]
            if pin_names(obj.get_editor_property("inputs")) == pin_names(props["inputs"]):
                del props["inputs"]
        obj.set_editor_properties(props)
        return obj

    def sample(self, path, x, y):
        tex = load(path)
        expr = self.node(u.MaterialExpressionTextureSample, x, y, texture=tex)
        compression = tex.get_editor_property("compression_settings")
        sampler = {
            u.TextureCompressionSettings.TC_NORMALMAP: "NORMAL",
            u.TextureCompressionSettings.TC_MASKS: "MASKS",
            u.TextureCompressionSettings.TC_GRAYSCALE: "GRAYSCALE" if tex.srgb else "LINEAR_GRAYSCALE",
            u.TextureCompressionSettings.TC_ALPHA: "ALPHA",
        }.get(compression, "COLOR" if tex.srgb else "LINEAR_COLOR")
        expr.sampler_type = getattr(u.MaterialSamplerType, "SAMPLERTYPE_" + sampler)
        return expr

    def constant(self, value, pin, x, y):
        self.output(self.node(u.MaterialExpressionConstant, x, y, r=value), "", pin)

    @staticmethod
    def output_pin(pin):
        return getattr(u.MaterialProperty, "MP_" + pin)

    def output(self, node, channel, pin):
        if not edit.connect_material_property(node, channel, self.output_pin(pin)):
            raise RuntimeError("Cannot connect " + pin)

    def shader(self, label, code, inputs, x=0, y=0):
        pins = []
        for name, _, _ in inputs:
            desc = u.CustomInput()
            desc.set_editor_property("input_name", name)
            pins.append(desc)
        node = self.node(u.MaterialExpressionCustom, x, y, description=label, code=code,
                         output_type=u.CustomMaterialOutputType.CMOT_FLOAT3, inputs=pins)
        for name, source, channel in inputs:
            if not edit.connect_material_expressions(source, channel, node, name):
                raise RuntimeError("Cannot wire " + name)
        return node

    def save(self):
        errors = edit.recompile_material(self.mat)
        if errors:
            raise RuntimeError(self.mat.get_name() + ": " + ", ".join(map(str, errors)))
        store(self.mat)
        return self.mat


def hair_material():
    s = Surface("M_MaxHair")
    s.mat.set_editor_properties(dict(two_sided=True, blend_mode=u.BlendMode.BLEND_MASKED,
                                     shading_model=u.MaterialShadingModel.MSM_DEFAULT_LIT,
                                     dither_opacity_mask=False, opacity_mask_clip_value=.38))
    # LOD0 of the Creator groom uses Card Compact, bInvertUV=False. The original
    # static mesh has one UV channel. The groom renderer inserts its own root UV
    # ahead of that atlas UV; the socket-mounted static mesh samples UV0 directly.
    atlas = s.sample(HAIR + "_CardsAtlas_Attribute", -640, -210)
    flow = s.sample(HAIR + "_CardsAtlas_Tangent", -640, 190)
    color = s.shader("Chestnut strands", """
        float rootTip = smoothstep(.06, .93, U);
        float variation = saturate(.33 + .38*A.b + .16*A.g + .13*rootTip);
        return lerp(float3(.011,.0058,.0039), float3(.066,.035,.020), variation);
    """, [("A", atlas, "RGB"), ("U", flow, "A")])
    s.output(color, "", "BASE_COLOR")
    ribbon = s.node(u.MaterialExpressionVertexColor, -640, -500)
    mask = s.shader("Overlapping locks with fine strand margins", """
        float edge = 1.0 - abs(2.0*Ribbon.x-1.0);
        float tip = smoothstep(.68,.99,Ribbon.y);
        float center = smoothstep(.10,.37+.35*tip,edge);
        float separation = .10*sin(104*Ribbon.x+3*U+5*A.b);
        float undercoat = (.80-.39*tip+.14*A.g+separation)*center;
        return saturate(max(pow(A.r,.62),undercoat));
    """, [("A",atlas,"RGB"),("U",flow,"A"),("Ribbon",ribbon,"")], x=0,y=-480)
    s.output(mask,"","OPACITY_MASK")
    direction = s.shader("Groom strand direction in the card tangent frame", """
        return normalize(Flow*2.0-1.0);
    """, [("Flow", flow, "RGB")], x=0, y=510)
    s.output(direction, "", "TANGENT")
    normal=s.node(u.MaterialExpressionConstant3Vector, -250, 510, constant=u.LinearColor(0,0,1,1))
    s.output(normal,"","NORMAL")
    s.constant(.62, "ROUGHNESS", 0, 220)
    s.constant(.25, "SPECULAR", 0, 330)
    s.constant(.48, "ANISOTROPY", 0, 690)
    return s.save()


def geometry(skeletal):
    dm = u.DynamicMesh()
    _, result = u.GeometryScript_AssetUtils.copy_mesh_from_skeletal_mesh(
        skeletal, dm, u.GeometryScriptCopyMeshFromAssetOptions(), u.GeometryScriptMeshReadLOD())
    if result != u.GeometryScriptOutcomePins.SUCCESS:
        raise RuntimeError("Could not read " + skeletal.get_name())
    positions = dm.get_all_vertex_positions(False)[-2]
    return u.GeometryScript_List.convert_vector_list_to_array(positions)


def fit_hair(mat):
    original = load(PLUGIN + "/Face/SKM_Face")
    head = load(ROOT + "/Max_Head")
    original_points, edited_points = geometry(original), geometry(head)
    if len(original_points) != len(edited_points):
        raise RuntimeError("Expected matching Creator face topology.")
    # Fit from homologous upper-skull vertices so changes in stature and cranial
    # proportions move the hairline together. Exclude the eyes, jaw and neck.
    pairs = [(p, q) for p, q in zip(original_points, edited_points) if p.z > 161.0 and p.y < 6.5]
    if len(pairs) < 100:
        raise RuntimeError("No matching scalp region.")
    def stats(points):
        axes = list(zip(*(p.to_tuple() for p in points)))
        c = [sum(axis) / len(axis) for axis in axes]
        dev = [math.sqrt(sum((v-m)**2 for v in axis) / len(axis)) for axis, m in zip(axes, c)]
        return c, dev
    old, old_std = stats([p for p, _ in pairs])
    new, new_std = stats([q for _, q in pairs])
    scale = [max(.88, min(1.12, b / a)) for a, b in zip(old_std, new_std)]
    translation = [b-s*a for a, b, s in zip(old, new, scale)]
    bind = u.SkeletonModifier()
    bind.set_skeletal_mesh(head)
    mesh = u.DynamicMesh()
    _, result = u.GeometryScript_AssetUtils.copy_mesh_from_static_mesh(load(HAIR + "_CardsMesh_Group0_LOD0"),
        mesh, u.GeometryScriptCopyMeshFromAssetOptions(), u.GeometryScriptMeshReadLOD())
    if result != u.GeometryScriptOutcomePins.SUCCESS:
        raise RuntimeError("Cannot read Creator hair cards")
    # These imported strips sometimes end with a full-width quad in LOD0.
    # Taper each strip towards the end indicated by the packed CoordU gradient.
    # Preserve UV0: it contains the actual atlas rectangle for all the fine hairs.
    points = list(u.GeometryScript_List.convert_vector_list_to_array(mesh.get_all_vertex_positions(False)[-2]))
    triangles = u.GeometryScript_List.convert_triangle_list_to_array(mesh.get_all_triangle_indices(False)[-2])
    parents = list(range(len(points)))
    atlas_v = [0.0]*len(points)
    atlas_u = [0.0]*len(points)
    ribbon_colors = [u.LinearColor(.5,0,0,1)]*len(points)
    def root(i):
        while parents[i] != i:
            parents[i] = parents[parents[i]]
            i = parents[i]
        return i
    for i,triangle in enumerate(triangles):
        ids = (triangle.x,triangle.y,triangle.z)
        coords = mesh.get_triangle_u_vs(0,i)
        for index,uv in zip(ids,coords[:3]):
            parents[root(index)] = root(ids[0])
            atlas_v[index] = uv.y
            atlas_u[index] = uv.x
    ribbons = {}
    for i,v in enumerate(atlas_v):
        ribbons.setdefault(root(i),{}).setdefault(round(v,5),[]).append(i)
    for rows in ribbons.values():
        lo,hi = min(rows), max(rows)
        if hi-lo<.001 or len(rows)<3:
            continue
        umin=min(atlas_u[i] for row in rows.values() for i in row)
        umax=max(atlas_u[i] for row in rows.values() for i in row)
        for v,indices in rows.items():
            for i in indices:
                ribbon_colors[i]=u.LinearColor((atlas_u[i]-umin)/max(.00001,umax-umin),(v-lo)/(hi-lo),0,1)
            if len(indices)<2:
                continue
            along = max(0,min(1,((v-lo)/(hi-lo)-.66)/.34))
            if not along:
                continue
            ease = along*along*(3-2*along)
            middle = sum((points[i] for i in indices),u.Vector()) / len(indices)
            for i in indices:
                points[i] = middle + (points[i]-middle)*(1-.92*ease)
    mesh.set_mesh_per_vertex_colors(u.GeometryScript_List.convert_array_to_color_list(ribbon_colors))
    mesh.set_all_mesh_vertex_positions(u.GeometryScript_List.convert_array_to_vector_list(points))
    mesh.transform_mesh(u.Transform(location=translation, scale=scale))
    mesh.inverse_transform_mesh(bind.get_bone_transform("head", True))
    name = ROOT + "/SM_MaxHair"
    if assets.does_asset_exist(name):
        target = load(name)
        _, result = u.GeometryScript_AssetUtils.copy_mesh_to_static_mesh(
            mesh, target, u.GeometryScriptCopyMeshToAssetOptions(), u.GeometryScriptMeshWriteLOD())
    else:
        opts = u.GeometryScriptCreateNewStaticMeshAssetOptions()
        opts.set_editor_property("enable_collision", False)
        target, result = u.GeometryScript_NewAssetUtils.create_new_static_mesh_asset_from_mesh(mesh, name, opts)
    if result != u.GeometryScriptOutcomePins.SUCCESS:
        raise RuntimeError("Cannot save fitted hair")
    target.set_material(0, mat)
    store(target)
    u.log(f"MAX fitted {mesh.get_triangle_count()} hair triangles to {len(pairs)} matching scalp vertices")


def skin(region):
    s = Surface("M_Max" + region)
    s.mat.set_editor_property("used_with_skeletal_mesh", True)
    prefix = ROOT + "/Textures/T_Max_" + region + "_"
    col = s.sample(prefix + "BaseColor", -740, -320)
    normal = s.sample(prefix + "Normal", -740, 290)
    cavity = s.sample(prefix + "Cavity", -740, 535)
    s.output(normal, "RGB", "NORMAL")
    s.output(cavity, "R", "AMBIENT_OCCLUSION")
    if region == "Face":
        s.mat.set_editor_properties({
            "shading_model": u.MaterialShadingModel.MSM_SUBSURFACE_PROFILE,
            "subsurface_profile": load(PLUGIN + "/Lookdev_UHM/Skin/Materials/SSP_skin_unified"),
        })
        def mask(folder, style, y):
            return s.sample(f"{GROOMS}/{folder}/{style}/{style}_baked", -740, y)
        inputs = [("Skin", col, "RGB"), ("Scalp", mask("Hair", "Hair_S_Messy", -100), "A"),
                  ("Brows", mask("Eyebrows", "Eyebrows_M_Natural", 700), "A"),
                  ("Beard", mask("Beards", "Beard_S_Stubble", 950), "A"),
                  ("Lip", mask("Mustaches", "Mustache_S_Stubble", 1200), "A")]
        tinted = s.shader("Natural brows, short stubble and hairline", """
            // Side Swept Fringe ships without its own scalp bake. This short
            // temple undercoat fills the hairline beneath the fitted cards.
            float hairs = saturate(max(max(Scalp * 1.60, pow(Brows, .56) * 1.45),
                                       max(pow(Beard, .71) * .97, pow(Lip, .71))));
            return lerp(Skin, float3(.030,.015,.008), hairs);
        """, inputs)
        s.output(tinted, "", "BASE_COLOR")
        s.constant(.68, "OPACITY", 0, 720)
    else:
        # Creator's body albedo stores normalized skin variation. These are the
        # same linear reconstruction coefficients as MF_skin_UI_overrides_input.
        settings = load("/Game/NightOfTheWolves/Max/Creator/Max").skin_settings.skin
        coefficients = []
        for label in ["Bias", "Gain"]:
            value = settings.get_editor_property("body_" + label.lower())
            parameter = s.node(u.MaterialExpressionVectorParameter, -740, -120 + 140 * len(coefficients),
                               parameter_name="Skin " + label,
                               default_value=u.LinearColor(value.x, value.y, value.z, 1))
            coefficients.append((label, parameter, "RGB"))
        body_color = s.shader("Reconstruct Creator body skin tone",
                              "return saturate((Bias + Gain * (6.0 * Albedo - 3.0)) / 255.0);",
                              [("Albedo", col, "RGB"), *coefficients])
        s.output(body_color, "", "BASE_COLOR")
    s.constant(.51, "ROUGHNESS", 0, 410)
    s.constant(.33, "SPECULAR", 0, 540)
    return s.save()


def assign_materials(face_mat, body_mat):
    surfaces = {
        "head": face_mat,
        "teeth": load(PLUGIN + "/Lookdev_UHM/Teeth/Materials/Baked/MI_Teeth_Baked"),
        "eyeleft": load(PLUGIN + "/Materials/MI_Eye_Left_MHC"),
        "eyeright": load(PLUGIN + "/Materials/MI_Eye_Right_MHC"),
        "eyelashes": load(PLUGIN + "/Materials/MI_Eyelashes_HigherLODs"),
        "eyeshell": load(PLUGIN + "/Lookdev_UHM/Eye/Materials/MI_eye_occlusion_unified"),
        "eyeedge": load(PLUGIN + "/Lookdev_UHM/Eye/Materials/MI_eye_lacrimal_fluid_unified"),
    }
    for asset in [ROOT + "/Max_Head", ROOT + "/Max_Body",
                  "/Game/NightOfTheWolves/Max/Wardrobe/SK_MaxExposedBody"]:
        if not assets.does_asset_exist(asset):
            continue
        skeletal = load(asset)
        mats = list(skeletal.materials)
        for slot in mats:
            key = str(slot.material_slot_name).lower().split("_")[0]
            slot.material_interface = (surfaces.get(key, load(PLUGIN + "/Materials/M_Hide"))
                                       if asset.endswith("Head") else body_mat)
        skeletal.materials = mats
        store(skeletal)


fit_hair(hair_material())
if os.environ.get("MAX_APPEARANCE") != "hair":
    assign_materials(skin("Face"), skin("Body"))
u.log("MAX APPEARANCE SAVED")
