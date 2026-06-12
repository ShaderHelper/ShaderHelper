import os
import re
import threading
import time

import ShaderHelper as Sh

from . import text_result


_state_lock = threading.RLock()


def _sanitize_name(name, fallback="GeneratedEffect"):
    value = re.sub(r'[<>:"/\\|?*\x00-\x1f]', "", str(name or "")).strip()
    value = re.sub(r"\s+", "_", value)
    return value or fallback


def _current_asset_dir():
    cur_dir = Sh.Context.CurAssetBrowserDir
    if not cur_dir:
        raise ValueError("No active asset browser directory is available.")
    return os.path.abspath(cur_dir)


def _resolve_target_dir(target_dir=None):
    base_dir = _current_asset_dir()
    if target_dir:
        candidate = target_dir
        if not os.path.isabs(candidate):
            candidate = os.path.join(base_dir, candidate)
        candidate = os.path.abspath(candidate)
    else:
        candidate = base_dir

    base_cmp = os.path.normcase(base_dir)
    candidate_cmp = os.path.normcase(candidate)
    try:
        common = os.path.commonpath([base_cmp, candidate_cmp])
    except ValueError:
        common = ""
    if common != base_cmp:
        raise ValueError("target_dir must be inside the current ShaderHelper asset browser directory.")

    os.makedirs(candidate, exist_ok=True)
    return candidate


def _unique_path(directory, stem, extension, overwrite=False):
    path = os.path.join(directory, f"{stem}.{extension}")
    if overwrite or not os.path.exists(path):
        return path

    index = 1
    while True:
        path = os.path.join(directory, f"{stem}_{index}.{extension}")
        if not os.path.exists(path):
            return path
        index += 1


def _resolve_feedback_dir(output_dir=None):
    base_dir = os.path.abspath(Sh.PathHelper.SavedCaptureDir)
    if output_dir:
        candidate = output_dir
        if not os.path.isabs(candidate):
            candidate = os.path.join(base_dir, candidate)
        candidate = os.path.abspath(candidate)
    else:
        candidate = os.path.join(base_dir, "MCP")

    base_cmp = os.path.normcase(base_dir)
    candidate_cmp = os.path.normcase(candidate)
    try:
        common = os.path.commonpath([base_cmp, candidate_cmp])
    except ValueError:
        common = ""
    if common != base_cmp:
        raise ValueError("output_dir must be inside ShaderHelper's saved capture directory.")

    os.makedirs(candidate, exist_ok=True)
    return candidate


def _language_from_string(value):
    lang = str(value or "GLSL").strip().upper()
    if lang == "HLSL":
        return Sh.GpuShaderLanguage.HLSL
    if lang == "GLSL":
        return Sh.GpuShaderLanguage.GLSL
    raise ValueError("language must be GLSL or HLSL.")


def _slot_type_from_string(value):
    slot = str(value or "Texture2D").strip().lower()
    if slot in ("texture2d", "texture", "2d"):
        return Sh.ShaderToySlotType.Texture2D
    if slot in ("texturecube", "cubemap", "cube"):
        return Sh.ShaderToySlotType.TextureCube
    if slot in ("texture3d", "volume", "3d"):
        return Sh.ShaderToySlotType.Texture3D
    raise ValueError(f"Unsupported channel slot type: {value}")


def _filter_from_string(value):
    filter_name = str(value or "linear").strip().lower()
    if filter_name == "nearest":
        return Sh.ShaderToyFilterMode.Nearest
    if filter_name == "mipmap":
        return Sh.ShaderToyFilterMode.Mipmap
    if filter_name == "linear":
        return Sh.ShaderToyFilterMode.Linear
    raise ValueError(f"Unsupported sampler filter: {value}")


def _wrap_from_string(value):
    wrap_name = str(value or "repeat").strip().lower()
    if wrap_name == "clamp":
        return Sh.ShaderToyWrapMode.Clamp
    if wrap_name == "repeat":
        return Sh.ShaderToyWrapMode.Repeat
    raise ValueError(f"Unsupported sampler wrap: {value}")


def _set_channel_desc(pass_node, channel_index, channel):
    desc = getattr(pass_node, f"iChannel{channel_index}")
    desc.Filter = _filter_from_string(channel.get("filter", "linear"))
    desc.Wrap = _wrap_from_string(channel.get("wrap", "repeat"))


def _asset_from_path(path):
    abs_path = os.path.abspath(path)
    return Sh.Asset.LoadAsset(abs_path)


def _find_output_node(graph):
    for node in graph.Nodes:
        if isinstance(node, Sh.ShaderToyOutputNode):
            return node
    output_node = Sh.ShaderToyOutputNode(graph)
    graph.AddNode(output_node)
    return output_node


def _builtin_resource_path(kind, name):
    kind_map = {
        "texture": ("Texture", "texture"),
        "texture2d": ("Texture", "texture"),
        "cubemap": ("Cubemap", "cubemap"),
        "cube": ("Cubemap", "cubemap"),
        "volume": ("Volume", "texture3d"),
        "texture3d": ("Volume", "texture3d"),
    }
    normalized = str(kind or "texture").strip().lower()
    if normalized not in kind_map:
        raise ValueError(f"Unsupported builtin resource kind: {kind}")
    folder, extension = kind_map[normalized]
    return os.path.join(Sh.PathHelper.BuiltinDir, "ShaderToy", folder, f"{name}.{extension}")


def _make_texture_node(graph, source_path):
    asset = _asset_from_path(source_path)
    if asset is None:
        raise ValueError(f"Could not load texture asset: {source_path}")

    ext = os.path.splitext(source_path)[1].lower()
    if ext == ".cubemap":
        node = Sh.TextureCubeNode(graph, asset)
    elif ext == ".texture3d":
        node = Sh.Texture3dNode(graph, asset)
    else:
        node = Sh.Texture2dNode(graph, asset)
    graph.AddNode(node)
    return node


def _pass_name(value, fallback):
    name = str(value or fallback).strip()
    return name or fallback


def _as_list(value, fallback=None):
    if value is None:
        return list(fallback or [])
    if isinstance(value, (list, tuple)):
        return list(value)
    return [value]


def _enum_token(value):
    text = str(value or "").strip()
    if "." in text:
        text = text.rsplit(".", 1)[-1]
    return text.lower()


def _stage_name(value):
    stage = _enum_token(value)
    if stage in ("vs", "vertex"):
        return "vertex"
    if stage in ("ps", "fs", "pixel", "fragment"):
        return "pixel"
    if stage in ("cs", "compute"):
        return "compute"
    raise ValueError(f"Unsupported shader stage: {value}")


def _stage_enum(value):
    stage = _stage_name(value)
    if stage == "vertex":
        return Sh.ShaderType.Vertex
    if stage == "pixel":
        return Sh.ShaderType.Pixel
    if stage == "compute":
        return Sh.ShaderType.Compute
    raise ValueError(f"Unsupported shader stage: {value}")


def _default_entry_point(stage, language):
    if language == Sh.GpuShaderLanguage.GLSL:
        return "main"
    if stage == "vertex":
        return "MainVS"
    if stage == "pixel":
        return "MainPS"
    if stage == "compute":
        return "MainCS"
    return "main"


def _default_stage_macro(stage, language):
    if language != Sh.GpuShaderLanguage.GLSL:
        return []
    return [stage.upper()]


def _asset_compile_summary(asset, do_compile=False):
    result = {
        "isShaderAsset": isinstance(asset, Sh.ShaderAsset),
        "isCompilationSuccessful": None,
        "enabledStages": [],
    }
    if not isinstance(asset, Sh.ShaderAsset):
        return result

    result["isCompilationSuccessful"] = bool(asset.IsCompilationSuccessful())
    result["enabledStages"] = [_stage_name(stage) for stage in asset.GetEnabledStages()]

    if do_compile:
        compile_result = dict(asset.CompileShader())
        result["compile"] = {
            "success": bool(compile_result["success"]),
            "error": str(compile_result["error"]),
            "warning": str(compile_result["warning"]),
        }
        result["isCompilationSuccessful"] = bool(asset.IsCompilationSuccessful())
    return result


def _set_native_shader_options(shader, arguments, language):
    if not isinstance(shader, Sh.Shader):
        return

    stages = [_stage_name(stage) for stage in _as_list(arguments.get("stages"), ["vertex", "pixel"])]
    stage_values = [_stage_enum(stage) for stage in stages]
    shader.SetStages(stage_values)

    entry_points = arguments.get("entry_points") or {}
    stage_macros = arguments.get("stage_macros") or {}
    for stage, stage_value in zip(stages, stage_values):
        shader.SetEntryPoint(stage_value, entry_points.get(stage, _default_entry_point(stage, language)))
        shader.SetStageMacros(stage_value, stage_macros.get(stage, _default_stage_macro(stage, language)))


def _create_shader_asset(arguments, asset_type):
    base_name = _sanitize_name(arguments.get("name"), "Shader")
    target_dir = _resolve_target_dir(arguments.get("target_dir"))
    overwrite = bool(arguments.get("overwrite", False))
    language = _language_from_string(arguments.get("language", "HLSL"))
    code = arguments.get("code", "")

    normalized = str(asset_type or "shader").strip().lower()
    if normalized == "shader":
        class_type = Sh.Shader
    elif normalized == "st_shader":
        class_type = Sh.StShader
    elif normalized == "shader_header":
        class_type = Sh.ShaderHeader
    else:
        raise ValueError(f"Unsupported shader asset type: {asset_type}")

    shader = Sh.Asset.CreateAsset(base_name, class_type)
    shader.EditorContent = code
    shader.Language = language
    if isinstance(shader, Sh.StShader):
        slot_values = list(arguments.get("channel_slot_types") or ["Texture2D"] * 4)
        while len(slot_values) < 4:
            slot_values.append("Texture2D")
        shader.ChannelSlotTypes = tuple(_slot_type_from_string(slot_values[i]) for i in range(4))
    _set_native_shader_options(shader, arguments, language)

    shader_path = _unique_path(target_dir, base_name, shader.FileExtension, overwrite)
    Sh.Asset.SaveToFile(shader, shader_path)
    compile_info = _asset_compile_summary(shader, do_compile=bool(arguments.get("compile", False)))
    return shader, shader_path, compile_info


def _find_render_output_node(graph):
    for node in graph.Nodes:
        if isinstance(node, Sh.RenderOutputNode):
            return node
    output_node = Sh.RenderOutputNode(graph)
    graph.AddNode(output_node)
    return output_node


def _load_typed_asset(path, class_type, label):
    asset = _asset_from_path(path)
    if asset is None:
        raise ValueError(f"Could not load {label}: {path}")
    if not isinstance(asset, class_type):
        raise ValueError(f"Asset is not a {label}: {path}")
    return asset


def _vec3(value, label):
    if value is None:
        return None
    if not isinstance(value, (list, tuple)) or len(value) < 3:
        raise ValueError(f"{label} must contain x, y, and z.")
    return (float(value[0]), float(value[1]), float(value[2]))


def _set_scene_transform(scene_object, spec):
    position = _vec3(spec.get("position"), "position")
    rotation = _vec3(spec.get("rotation"), "rotation")
    scale = _vec3(spec.get("scale"), "scale")
    if position is not None:
        scene_object.Position = position
    if rotation is not None:
        scene_object.Rotation = rotation
    if scale is not None:
        scene_object.Scale = scale


def _scene_object_key(spec, index):
    key = spec.get("id") or spec.get("name") or f"SceneObject{index}"
    return _sanitize_name(key, f"SceneObject{index}")


def _scene_object_type(value):
    kind = str(value or "scene").strip().lower()
    if kind in ("scene", "scene_object", "object", "empty", "null"):
        return "scene"
    if kind in ("mesh", "mesh_scene_object", "meshsceneobject"):
        return "mesh"
    if kind in ("camera", "camera_scene_object", "camerasceneobject"):
        return "camera"
    raise ValueError(f"Unsupported scene object type: {value}")


def _create_scene_objects(graph, scene_specs):
    scene_specs = list(scene_specs or [])
    if not scene_specs:
        return {}, []

    scene_objects = {}
    summaries = []

    for index, spec in enumerate(scene_specs):
        if not isinstance(spec, dict):
            raise ValueError("scene_objects entries must be objects.")

        key = _scene_object_key(spec, index)
        if key in scene_objects:
            raise ValueError(f"Duplicate scene object id/name: {key}")

        parent_name = spec.get("parent") or spec.get("parent_id")
        parent = None
        if parent_name:
            parent_key = _sanitize_name(parent_name)
            if parent_key not in scene_objects:
                raise ValueError(f"Scene object parent does not exist or appears later: {parent_name}")
            parent = scene_objects[parent_key]

        kind = _scene_object_type(spec.get("type"))
        if kind == "mesh":
            scene_object = graph.AddMeshSceneObject(parent)
        elif kind == "camera":
            scene_object = graph.AddCameraSceneObject(parent)
        else:
            scene_object = graph.AddSceneObject(parent)

        display_name = spec.get("name") or key
        scene_object.SetName(str(display_name))
        _set_scene_transform(scene_object, spec)

        item = {
            "id": key,
            "name": scene_object.Name,
            "type": type(scene_object).__name__,
            "parent": parent_name,
        }

        if kind == "mesh":
            model_path = spec.get("model_path") or spec.get("model")
            if model_path:
                scene_object.ModelAsset = _load_typed_asset(model_path, Sh.Model, "Model")
                item["model"] = os.path.abspath(model_path)
            if spec.get("vertex_count") is not None:
                scene_object.VertexCount = int(spec["vertex_count"])
            if spec.get("instance_count") is not None:
                scene_object.InstanceCount = int(spec["instance_count"])
            item["vertexCount"] = int(scene_object.VertexCount)
            item["instanceCount"] = int(scene_object.InstanceCount)

        if kind == "camera":
            if spec.get("orthographic") is not None:
                scene_object.Orthographic = bool(spec["orthographic"])
            if spec.get("ortho_size") is not None:
                scene_object.OrthoSize = float(spec["ortho_size"])
            if spec.get("vertical_fov") is not None:
                scene_object.VerticalFov = float(spec["vertical_fov"])
            if spec.get("near_plane") is not None:
                scene_object.NearPlane = float(spec["near_plane"])
            if spec.get("far_plane") is not None:
                scene_object.FarPlane = float(spec["far_plane"])
            if bool(spec.get("preview", False)):
                graph.PreviewCamera = scene_object
            item["orthographic"] = bool(scene_object.Orthographic)

        scene_objects[key] = scene_object
        summaries.append(item)

    return scene_objects, summaries


def _mesh_binding_specs(spec):
    bindings = spec.get("meshes") or spec.get("mesh_objects") or spec.get("render_objects")
    if bindings is None:
        mesh_ref = spec.get("mesh") or spec.get("scene_object") or spec.get("mesh_scene_object")
        if mesh_ref:
            bindings = [{
                "scene_object": mesh_ref,
                "material_path": spec.get("material_path"),
                "material_paths": spec.get("material_paths"),
            }]
    result = []
    for binding in bindings or []:
        if isinstance(binding, str):
            result.append({"scene_object": binding})
        elif isinstance(binding, dict):
            result.append(binding)
        else:
            raise ValueError("mesh pass meshes entries must be objects or scene object names.")
    return result


def _material_paths_from_binding(binding, fallback_path=None):
    paths = binding.get("material_paths") or binding.get("materials")
    if paths is not None:
        if isinstance(paths, str):
            return [paths] if paths else []
        return [path for path in paths if path]
    path = binding.get("material_path") or binding.get("material") or fallback_path
    return [path] if path else []


def _bind_mesh_pass_scene(node, spec, scene_objects):
    if not scene_objects and not _mesh_binding_specs(spec) and not spec.get("camera"):
        return []

    camera_name = spec.get("camera") or spec.get("camera_ref") or spec.get("camera_object")
    if camera_name:
        camera_key = _sanitize_name(camera_name)
        camera = scene_objects.get(camera_key)
        if camera is None:
            raise ValueError(f"Mesh pass camera scene object does not exist: {camera_name}")
        if not isinstance(camera, Sh.CameraSceneObject):
            raise ValueError(f"Mesh pass camera must reference a CameraSceneObject: {camera_name}")
        node.CameraRef = camera

    bindings = []
    fallback_material = spec.get("material_path") or spec.get("material")
    for binding in _mesh_binding_specs(spec):
        mesh_name = binding.get("scene_object") or binding.get("mesh") or binding.get("object") or binding.get("name")
        if not mesh_name:
            raise ValueError("mesh pass mesh binding is missing scene_object.")

        mesh_key = _sanitize_name(mesh_name)
        mesh_object = scene_objects.get(mesh_key)
        if mesh_object is None:
            raise ValueError(f"Mesh scene object does not exist: {mesh_name}")
        if not isinstance(mesh_object, Sh.MeshSceneObject):
            raise ValueError(f"Mesh binding must reference a MeshSceneObject: {mesh_name}")

        render_objects = list(node.AddMeshRenderObject(mesh_object))
        material_paths = _material_paths_from_binding(binding, fallback_material)
        material_assets = [_load_typed_asset(path, Sh.Material, "Material") for path in material_paths if path]

        for index, render_object in enumerate(render_objects):
            if material_assets:
                material = material_assets[index] if index < len(material_assets) else material_assets[-1]
                render_object.MaterialAsset = material
            bindings.append({
                "mesh": mesh_key,
                "meshRenderObject": render_object.Name,
                "subMeshIndex": int(render_object.SubMeshIndex),
                "material": os.path.abspath(material_paths[min(index, len(material_paths) - 1)]) if material_paths else None,
            })

    return bindings


def _pin_name(value, default):
    return str(value or default)


def _pin_direction(value, default):
    direction = _enum_token(value or default)
    if direction in ("input", "in", "target", "to"):
        return Sh.PinDirection.Input, "input"
    if direction in ("output", "out", "source", "from"):
        return Sh.PinDirection.Output, "output"
    raise ValueError(f"Unsupported pin direction: {value}")


def _pin_direction_name(value, default="output"):
    _, name = _pin_direction(value, default)
    return name


def _node_pin(node, pin_name, direction, default_direction, node_name):
    name = _pin_name(pin_name, "RT")
    pin_direction, pin_direction_name = _pin_direction(direction, default_direction)
    pin = node.GetPin(name, pin_direction)
    if pin is None:
        raise ValueError(f"Node {node_name} has no {pin_direction_name} pin named {name}.")
    return pin


def tool_shaderhelper_status(arguments):
    from . import bridge_status

    graph = Sh.Context.Graph
    bridge = bridge_status()
    return text_result({
        "bridge": bridge,
        "assetBrowserDir": _current_asset_dir(),
        "currentGraph": {
            "available": graph is not None,
            "name": graph.Name if graph is not None else None,
            "type": type(graph).__name__ if graph is not None else None,
        },
        "tools": bridge["tools"],
        "shaderDebuggingHint": (
            "Native Shader assets can use Print/PrintAtMouse/Assert in HLSL or GLSL. "
            "Compile, then call render_graph_feedback to inspect viewport output, shader logs, prints, and asserts."
        ),
    })


def tool_list_builtin_shadertoy_resources(arguments):
    root = os.path.join(Sh.PathHelper.BuiltinDir, "ShaderToy")
    groups = {
        "textures": ("Texture", ".texture"),
        "cubemaps": ("Cubemap", ".cubemap"),
        "volumes": ("Volume", ".texture3d"),
    }
    result = {}
    for key, (folder, extension) in groups.items():
        directory = os.path.join(root, folder)
        items = []
        if os.path.isdir(directory):
            for filename in sorted(os.listdir(directory)):
                if filename.lower().endswith(extension):
                    items.append({
                        "name": os.path.splitext(filename)[0],
                        "path": os.path.join(directory, filename),
                    })
        result[key] = items
    return text_result(result)


def tool_create_shader_asset(arguments):
    with _state_lock:
        asset_type = arguments.get("asset_type", "shader")
        shader, shader_path, compile_info = _create_shader_asset(arguments, asset_type)
        return text_result({
            "path": shader_path,
            "assetType": type(shader).__name__,
            "language": shader.Language.name,
            "compile": compile_info,
            "message": "Shader asset created.",
        })


def tool_create_shadertoy_graph(arguments):
    with _state_lock:
        base_name = _sanitize_name(arguments.get("name"), "ShadertoyGraph")
        target_dir = _resolve_target_dir(arguments.get("target_dir"))
        overwrite = bool(arguments.get("overwrite", False))
        graph = Sh.Asset.CreateAsset(base_name, Sh.ShaderToy)
        graph.FlipY = bool(arguments.get("flip_y", True))

        pass_nodes = {}
        pass_paths = {}
        for index, pass_spec in enumerate(arguments.get("passes") or []):
            pass_name = _pass_name(pass_spec.get("name"), "Image" if index == 0 else f"Pass{index}")
            if pass_name in pass_nodes:
                raise ValueError(f"Duplicate pass name: {pass_name}")
            shader_path = pass_spec.get("path") or pass_spec.get("shader_path")
            if not shader_path:
                raise ValueError(f"Pass {pass_name} is missing shader path.")
            shader = _load_typed_asset(shader_path, Sh.StShader, "StShader")
            node = Sh.ShaderToyPassNode(graph, shader)
            graph.AddNode(node)
            pass_nodes[pass_name] = node
            pass_paths[pass_name] = os.path.abspath(shader_path)

        if not pass_nodes:
            raise ValueError("passes must contain at least one Shadertoy pass shader.")

        texture_nodes = {}
        for channel in arguments.get("channels") or []:
            pass_name = channel.get("pass") or channel.get("pass_name") or channel.get("target_pass")
            if pass_name not in pass_nodes:
                raise ValueError(f"Channel target pass does not exist: {pass_name}")
            node = pass_nodes[pass_name]
            channel_index = int(channel.get("index", channel.get("channel", 0)))
            if channel_index < 0 or channel_index > 3:
                raise ValueError("channel index must be between 0 and 3.")
            _set_channel_desc(node, channel_index, channel)

            source_node = None
            source_pass = channel.get("source_pass") or channel.get("source")
            previous_frame = bool(channel.get("previous_frame", False))
            if source_pass in pass_nodes:
                source_node = pass_nodes[source_pass]
                if source_node is node:
                    previous_frame = True
            else:
                source_path = channel.get("asset_path")
                if not source_path and channel.get("builtin"):
                    builtin = channel["builtin"]
                    if isinstance(builtin, dict):
                        source_path = _builtin_resource_path(builtin.get("kind", "texture"), builtin.get("name"))
                    else:
                        source_path = _builtin_resource_path(channel.get("kind", "texture"), builtin)
                if source_path:
                    source_path = os.path.abspath(source_path)
                    if source_path not in texture_nodes:
                        texture_nodes[source_path] = _make_texture_node(graph, source_path)
                    source_node = texture_nodes[source_path]

            if source_node is None:
                raise ValueError(f"Channel {channel_index} of pass {pass_name} has no valid source.")

            if previous_frame:
                source_node = Sh.ShaderToyPreviousFrameNode(graph, source_node)
                graph.AddNode(source_node)
            graph.AddLink(source_node.GetPin("RT"), node.GetPin(f"iChannel{channel_index}"))

        output_pass = arguments.get("output_pass") or ("Image" if "Image" in pass_nodes else next(reversed(pass_nodes)))
        if output_pass not in pass_nodes:
            raise ValueError(f"output_pass does not exist: {output_pass}")
        output_node = _find_output_node(graph)
        graph.AddLink(pass_nodes[output_pass].GetPin("RT"), output_node.GetPin("RT"))

        graph_path = _unique_path(target_dir, base_name, graph.FileExtension, overwrite)
        Sh.Asset.SaveToFile(graph, graph_path)
        return text_result({
            "graph": graph_path,
            "passes": pass_paths,
            "outputPass": output_pass,
            "message": "Shadertoy graph asset created.",
        })


def tool_compile_shader_asset(arguments):
    with _state_lock:
        path = arguments.get("path")
        if not path:
            raise ValueError("path is required.")
        asset = _load_typed_asset(path, Sh.ShaderAsset, "ShaderAsset")
        compile_info = _asset_compile_summary(asset, do_compile=bool(arguments.get("compile", True)))
        return text_result({
            "path": os.path.abspath(path),
            "fileName": asset.FileName,
            "fileExtension": asset.FileExtension,
            "language": asset.Language.name,
            **compile_info,
        })


def tool_create_material_asset(arguments):
    with _state_lock:
        base_name = _sanitize_name(arguments.get("name"), "Material")
        target_dir = _resolve_target_dir(arguments.get("target_dir"))
        overwrite = bool(arguments.get("overwrite", False))
        material = Sh.Asset.CreateAsset(base_name, Sh.Material)

        vertex_shader_path = arguments.get("vertex_shader_path")
        pixel_shader_path = arguments.get("pixel_shader_path")
        if vertex_shader_path:
            material.VertexShaderAsset = _load_typed_asset(vertex_shader_path, Sh.Shader, "Shader")
        if pixel_shader_path:
            material.PixelShaderAsset = _load_typed_asset(pixel_shader_path, Sh.Shader, "Shader")
        material.RefreshShaderBindings()

        material_path = _unique_path(target_dir, base_name, material.FileExtension, overwrite)
        Sh.Asset.SaveToFile(material, material_path)
        return text_result({
            "path": material_path,
            "vertexShader": os.path.abspath(vertex_shader_path) if vertex_shader_path else None,
            "pixelShader": os.path.abspath(pixel_shader_path) if pixel_shader_path else None,
            "message": "Material asset created.",
        })


def tool_create_render_graph(arguments):
    with _state_lock:
        base_name = _sanitize_name(arguments.get("name"), "RenderGraph")
        target_dir = _resolve_target_dir(arguments.get("target_dir"))
        overwrite = bool(arguments.get("overwrite", False))
        graph = Sh.Asset.CreateAsset(base_name, Sh.Render)
        scene_objects_by_name, scene_object_summaries = _create_scene_objects(graph, arguments.get("scene_objects") or arguments.get("scene"))

        nodes_by_name = {}
        mesh_bindings = []
        output_node = _find_render_output_node(graph)
        nodes_by_name["Output"] = output_node

        for index, spec in enumerate(arguments.get("nodes") or []):
            node_name = _pass_name(spec.get("name"), f"Node{index}")
            node_type = str(spec.get("type", "")).strip().lower()
            if node_name in nodes_by_name:
                raise ValueError(f"Duplicate node name: {node_name}")

            if node_type in ("render_output", "output", "present"):
                node = output_node
            elif node_type in ("texture", "texture2d", "texture_cube", "texture3d"):
                source_path = spec.get("asset_path") or spec.get("path")
                if not source_path:
                    raise ValueError(f"Texture node {node_name} is missing asset_path.")
                node = _make_texture_node(graph, source_path)
            elif node_type in ("compute", "compute_pass", "computepassnode"):
                shader_path = spec.get("shader_path")
                shader = _load_typed_asset(shader_path, Sh.Shader, "Shader") if shader_path else None
                if shader is not None:
                    node = Sh.ComputePassNode(graph, shader)
                else:
                    node = Sh.ComputePassNode(graph)
                if spec.get("thread_group_count"):
                    count = list(spec["thread_group_count"])
                    while len(count) < 3:
                        count.append(1)
                    node.SetThreadGroupCount(int(count[0]), int(count[1]), int(count[2]))
                graph.AddNode(node)
            elif node_type in ("mesh", "mesh_pass", "meshpassnode"):
                node = Sh.MeshPassNode(graph)
                if spec.get("color_target_count") is not None:
                    node.SetColorTargetCount(int(spec["color_target_count"]))
                if spec.get("depth_enabled") is not None:
                    node.DepthEnabled = bool(spec["depth_enabled"])
                if spec.get("rt_size"):
                    size = list(spec["rt_size"])
                    if len(size) < 2:
                        raise ValueError("rt_size must contain width and height.")
                    node.SetRenderTargetSize(int(size[0]), int(size[1]))
                graph.AddNode(node)
                for binding in _bind_mesh_pass_scene(node, spec, scene_objects_by_name):
                    binding["node"] = node_name
                    mesh_bindings.append(binding)
            else:
                raise ValueError(f"Unsupported render node type: {spec.get('type')}")

            nodes_by_name[node_name] = node

        for link in arguments.get("links") or []:
            from_name = link.get("from")
            to_name = link.get("to")
            if from_name not in nodes_by_name:
                raise ValueError(f"Link source node does not exist: {from_name}")
            if to_name not in nodes_by_name:
                raise ValueError(f"Link target node does not exist: {to_name}")
            source_node = nodes_by_name[from_name]
            target_node = nodes_by_name[to_name]
            source_pin = _node_pin(
                source_node,
                link.get("out_pin") or link.get("from_pin") or link.get("source_pin"),
                link.get("from_direction") or link.get("out_direction") or link.get("source_direction"),
                "output",
                from_name,
            )
            target_pin = _node_pin(
                target_node,
                link.get("in_pin") or link.get("to_pin") or link.get("target_pin"),
                link.get("to_direction") or link.get("in_direction") or link.get("target_direction"),
                "input",
                to_name,
            )
            graph.AddLink(
                source_pin,
                target_pin,
            )

        render_path = _unique_path(target_dir, base_name, graph.FileExtension, overwrite)
        Sh.Asset.SaveToFile(graph, render_path)
        return text_result({
            "graph": render_path,
            "nodes": list(nodes_by_name.keys()),
            "sceneObjects": scene_object_summaries,
            "meshBindings": mesh_bindings,
            "message": "Render graph asset created.",
        })


def _diagnostics_from_render_logs(logs):
    diagnostics = {
        "errors": [],
        "warnings": [],
        "prints": [],
        "asserts": [],
    }
    for log in logs:
        category = str(log.get("category", ""))
        verbosity = str(log.get("verbosity", "")).lower()
        message = str(log.get("message", ""))
        lower_message = message.lower()

        if verbosity in ("error", "fatal"):
            diagnostics["errors"].append(log)
        elif verbosity == "warning":
            diagnostics["warnings"].append(log)

        if category == "LogShader" and verbosity in ("display", "log"):
            diagnostics["prints"].append(log)

        if "assert failed" in lower_message or (verbosity in ("error", "fatal") and "assert" in lower_message):
            diagnostics["asserts"].append(log)

    return diagnostics


def tool_render_graph_feedback(arguments):
    with _state_lock:
        graph_path = arguments.get("graph_path") or arguments.get("path")
        if not graph_path:
            raise ValueError("graph_path is required.")

        abs_graph_path = os.path.abspath(graph_path)
        frames = max(1, int(arguments.get("frames", 1)))
        time_step = float(arguments.get("time_step", 1.0 / 60.0))
        capture_viewport = bool(arguments.get("capture_viewport", True))

        capture_path = ""
        if capture_viewport:
            output_dir = _resolve_feedback_dir(arguments.get("output_dir"))
            graph_stem = _sanitize_name(os.path.splitext(os.path.basename(abs_graph_path))[0], "Graph")
            stamp = time.strftime("%Y%m%d_%H%M%S")
            capture_path = _unique_path(output_dir, f"{graph_stem}_{stamp}", "png", bool(arguments.get("overwrite", False)))

        feedback = dict(Sh.RenderGraphFeedback(abs_graph_path, frames, capture_path, time_step))
        logs = [dict(item) for item in feedback.get("logs", [])]
        viewport = feedback.get("viewport")
        if viewport is not None:
            viewport = dict(viewport)
        performance = feedback.get("performance")
        pass_times = []
        if performance is not None:
            performance = dict(performance)
            pass_times = [dict(item) for item in performance.get("passes", [])]
            performance["passes"] = pass_times

        return text_result({
            "success": bool(feedback.get("success", False)),
            "graph": abs_graph_path,
            "framesRendered": feedback.get("framesRendered", 0),
            "viewport": viewport,
            "viewportImage": viewport.get("path") if viewport else None,
            "error": feedback.get("error") or None,
            "viewportError": feedback.get("viewportError") or None,
            "passTimes": pass_times,
            "performance": performance,
            "diagnostics": _diagnostics_from_render_logs(logs),
            "logs": logs,
            "message": "Graph rendered and feedback captured.",
        })


def tool_read_shader_asset(arguments):
    path = arguments.get("path")
    if not path:
        raise ValueError("path is required.")
    asset = _asset_from_path(path)
    if asset is None:
        raise ValueError(f"Could not load asset: {path}")
    if not isinstance(asset, Sh.ShaderAsset):
        raise ValueError("Asset is not a ShaderAsset.")
    return text_result({
        "path": os.path.abspath(path),
        "fileName": asset.FileName,
        "fileExtension": asset.FileExtension,
        "language": asset.Language.name,
        "editorContent": asset.EditorContent,
        **_asset_compile_summary(asset),
    })


def tool_update_shader_asset(arguments):
    with _state_lock:
        path = arguments.get("path")
        code = arguments.get("code")
        if not path:
            raise ValueError("path is required.")
        if code is None:
            raise ValueError("code is required.")

        abs_path = os.path.abspath(path)
        base_dir = _current_asset_dir()
        base_cmp = os.path.normcase(base_dir)
        path_cmp = os.path.normcase(abs_path)
        try:
            common = os.path.commonpath([base_cmp, path_cmp])
        except ValueError:
            common = ""
        if common != base_cmp:
            raise ValueError("path must be inside the current ShaderHelper asset browser directory.")

        asset = _asset_from_path(abs_path)
        if asset is None:
            raise ValueError(f"Could not load asset: {path}")
        if not isinstance(asset, Sh.ShaderAsset):
            raise ValueError("Asset is not a ShaderAsset.")

        asset.EditorContent = code
        if arguments.get("language"):
            asset.Language = _language_from_string(arguments["language"])
        if isinstance(asset, Sh.StShader) and arguments.get("channel_slot_types"):
            slot_values = list(arguments.get("channel_slot_types"))
            while len(slot_values) < 4:
                slot_values.append("Texture2D")
            asset.ChannelSlotTypes = tuple(_slot_type_from_string(slot_values[i]) for i in range(4))
        if isinstance(asset, Sh.Shader):
            _set_native_shader_options(asset, arguments, asset.Language)

        asset.Save()
        compile_info = _asset_compile_summary(asset, do_compile=bool(arguments.get("compile", False)))
        asset.RefreshShader()
        return text_result({
            "path": abs_path,
            "compile": compile_info,
            "message": "Shader asset updated.",
        })


def tool_inspect_current_graph(arguments):
    graph = Sh.Context.Graph
    if graph is None:
        raise ValueError("No graph is currently open.")

    scene_objects = []
    if isinstance(graph, Sh.Render):
        for scene_object in graph.SceneObjects:
            item = {
                "id": scene_object.Id,
                "name": scene_object.Name,
                "type": type(scene_object).__name__,
                "position": list(scene_object.Position),
                "rotation": list(scene_object.Rotation),
                "scale": list(scene_object.Scale),
            }
            if isinstance(scene_object, Sh.MeshSceneObject):
                item["vertexCount"] = int(scene_object.VertexCount)
                item["instanceCount"] = int(scene_object.InstanceCount)
                model = scene_object.ModelAsset
                item["model"] = model.Name if model is not None else None
            if isinstance(scene_object, Sh.CameraSceneObject):
                item["orthographic"] = bool(scene_object.Orthographic)
                item["verticalFov"] = float(scene_object.VerticalFov)
                item["nearPlane"] = float(scene_object.NearPlane)
                item["farPlane"] = float(scene_object.FarPlane)
            scene_objects.append(item)

    nodes = []
    for node in graph.Nodes:
        item = {
            "id": node.Id,
            "name": node.Name,
            "type": type(node).__name__,
            "inputs": [],
            "outputs": [],
        }
        if isinstance(node, Sh.ShaderToyPassNode):
            item["shaderToyCode"] = node.GetShaderToyCode()
        if isinstance(node, Sh.MeshPassNode):
            camera = node.CameraRef
            item["camera"] = camera.Name if camera is not None else None
            item["meshRenderObjects"] = []
            for render_object in node.MeshRenderObjects:
                mesh_object = render_object.MeshSceneObjectRef
                material = render_object.MaterialAsset
                item["meshRenderObjects"].append({
                    "name": render_object.Name,
                    "mesh": mesh_object.Name if mesh_object is not None else None,
                    "subMeshIndex": int(render_object.SubMeshIndex),
                    "material": material.Name if material is not None else None,
                })
        for pin in node.InputPins:
            source = pin.SourceNode
            item["inputs"].append({
                "name": pin.Name,
                "sourceNodeId": source.Id if source is not None else None,
                "sourceNodeName": source.Name if source is not None else None,
                "sourceNodeType": type(source).__name__ if source is not None else None,
            })
        for pin in node.OutputPins:
            item["outputs"].append({
                "name": pin.Name,
                "direction": _pin_direction_name(pin.Direction, "output"),
            })
        nodes.append(item)

    return text_result({
        "graph": {
            "id": graph.Id,
            "name": graph.Name,
            "type": type(graph).__name__,
        },
        "sceneObjects": scene_objects,
        "nodes": nodes,
    })
