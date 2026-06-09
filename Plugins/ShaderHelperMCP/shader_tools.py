import os
import re
import threading

import ShaderHelper as Sh

from . import text_result


_state_lock = threading.RLock()
_created_assets = {}


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
    if abs_path in _created_assets:
        return _created_assets[abs_path]
    asset = Sh.Asset.LoadAsset(abs_path)
    if asset is not None:
        _created_assets[abs_path] = asset
    return asset


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
    if hasattr(value, "name"):
        value = value.name
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
    if not hasattr(Sh, "ShaderType"):
        return stage
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
        "isCompilable": False,
        "isCompilationSuccessful": None,
        "enabledStages": [],
    }
    if not isinstance(asset, Sh.ShaderAsset):
        return result

    if hasattr(asset, "IsCompilable"):
        result["isCompilable"] = bool(asset.IsCompilable())
    if hasattr(asset, "IsCompilationSuccessful"):
        result["isCompilationSuccessful"] = bool(asset.IsCompilationSuccessful())
    if hasattr(asset, "GetEnabledStages"):
        result["enabledStages"] = [_stage_name(stage) for stage in asset.GetEnabledStages()]

    if do_compile:
        if not hasattr(asset, "CompileShader"):
            raise ValueError("ShaderAsset.CompileShader is not exposed by the running ShaderHelper build.")
        compile_result = dict(asset.CompileShader())
        result["compile"] = {
            "success": bool(compile_result["success"]),
            "error": str(compile_result["error"]),
            "warning": str(compile_result["warning"]),
        }
        if hasattr(asset, "IsCompilationSuccessful"):
            result["isCompilationSuccessful"] = bool(asset.IsCompilationSuccessful())
    return result


def _set_native_shader_options(shader, arguments, language):
    if not isinstance(shader, Sh.Shader):
        return

    stages = [_stage_name(stage) for stage in _as_list(arguments.get("stages"), ["vertex", "pixel"])]
    stage_values = [_stage_enum(stage) for stage in stages]
    if hasattr(shader, "SetStages"):
        shader.SetStages(stage_values)
    elif arguments.get("stages"):
        raise ValueError("Shader.SetStages is not exposed by the running ShaderHelper build.")

    entry_points = arguments.get("entry_points") or {}
    stage_macros = arguments.get("stage_macros") or {}
    for stage, stage_value in zip(stages, stage_values):
        if hasattr(shader, "SetEntryPoint"):
            shader.SetEntryPoint(stage_value, entry_points.get(stage, _default_entry_point(stage, language)))
        if hasattr(shader, "SetStageMacros"):
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
    if hasattr(shader, "Language"):
        shader.Language = language
    if isinstance(shader, Sh.StShader):
        slot_values = list(arguments.get("channel_slot_types") or ["Texture2D"] * 4)
        while len(slot_values) < 4:
            slot_values.append("Texture2D")
        shader.ChannelSlotTypes = tuple(_slot_type_from_string(slot_values[i]) for i in range(4))
    _set_native_shader_options(shader, arguments, language)

    shader_path = _unique_path(target_dir, base_name, shader.FileExtension, overwrite)
    Sh.Asset.SaveToFile(shader, shader_path)
    _created_assets[os.path.abspath(shader_path)] = shader
    compile_info = _asset_compile_summary(shader, do_compile=bool(arguments.get("compile", False)))
    return shader, shader_path, compile_info


def _find_render_output_node(graph):
    for node in graph.Nodes:
        if hasattr(Sh, "RenderOutputNode") and isinstance(node, Sh.RenderOutputNode):
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


def _pin_name(value, default):
    return str(value or default)


def _pin_direction(value, default):
    explicit = value is not None
    direction = _enum_token(value or default)
    if direction in ("input", "in", "target", "to"):
        if not hasattr(Sh, "PinDirection"):
            if explicit:
                raise ValueError("The running ShaderHelper build does not expose PinDirection. Rebuild ShaderHelper.")
            return None, "input"
        return Sh.PinDirection.Input, "input"
    if direction in ("output", "out", "source", "from"):
        if not hasattr(Sh, "PinDirection"):
            if explicit:
                raise ValueError("The running ShaderHelper build does not expose PinDirection. Rebuild ShaderHelper.")
            return None, "output"
        return Sh.PinDirection.Output, "output"
    raise ValueError(f"Unsupported pin direction: {value}")


def _pin_direction_name(value, default="output"):
    _, name = _pin_direction(value, default)
    return name


def _node_pin(node, pin_name, direction, default_direction, node_name):
    name = _pin_name(pin_name, "RT")
    pin_direction, pin_direction_name = _pin_direction(direction, default_direction)
    if pin_direction is None:
        pin = node.GetPin(name)
        if pin is None:
            raise ValueError(f"Node {node_name} has no pin named {name}.")
        return pin
    try:
        pin = node.GetPin(name, pin_direction)
    except TypeError as exc:
        if direction is not None:
            raise ValueError(
                "The running ShaderHelper build does not expose directional GraphNode.GetPin. "
                "Rebuild ShaderHelper or omit explicit pin direction."
            ) from exc
        pin = node.GetPin(name)
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
            "Compile, render, then inspect ShaderHelper logs and screenshots to debug visual output."
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
            "language": getattr(shader, "Language", None).name if hasattr(shader, "Language") else None,
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
        _created_assets[os.path.abspath(graph_path)] = graph
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
            "language": getattr(asset, "Language", None).name if hasattr(asset, "Language") else None,
            **compile_info,
        })


def tool_create_material_asset(arguments):
    with _state_lock:
        if not hasattr(Sh, "Material"):
            raise ValueError("Material is not exposed by the running ShaderHelper build.")

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
        if hasattr(material, "RefreshShaderBindings"):
            material.RefreshShaderBindings()

        material_path = _unique_path(target_dir, base_name, material.FileExtension, overwrite)
        Sh.Asset.SaveToFile(material, material_path)
        _created_assets[os.path.abspath(material_path)] = material
        return text_result({
            "path": material_path,
            "vertexShader": os.path.abspath(vertex_shader_path) if vertex_shader_path else None,
            "pixelShader": os.path.abspath(pixel_shader_path) if pixel_shader_path else None,
            "message": "Material asset created.",
        })


def tool_create_render_graph(arguments):
    with _state_lock:
        if not hasattr(Sh, "Render"):
            raise ValueError("Render is not exposed by the running ShaderHelper build.")

        base_name = _sanitize_name(arguments.get("name"), "RenderGraph")
        target_dir = _resolve_target_dir(arguments.get("target_dir"))
        overwrite = bool(arguments.get("overwrite", False))
        graph = Sh.Asset.CreateAsset(base_name, Sh.Render)

        nodes_by_name = {}
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
                if not hasattr(Sh, "ComputePassNode"):
                    raise ValueError("ComputePassNode is not exposed by the running ShaderHelper build.")
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
                if not hasattr(Sh, "MeshPassNode"):
                    raise ValueError("MeshPassNode is not exposed by the running ShaderHelper build.")
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
        _created_assets[os.path.abspath(render_path)] = graph
        return text_result({
            "graph": render_path,
            "nodes": list(nodes_by_name.keys()),
            "message": "Render graph asset created.",
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
        "language": getattr(asset, "Language", None).name if hasattr(asset, "Language") else None,
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
            _set_native_shader_options(asset, arguments, getattr(asset, "Language", Sh.GpuShaderLanguage.HLSL))

        Sh.Asset.SaveToFile(asset, abs_path)
        _created_assets[abs_path] = asset
        compile_info = _asset_compile_summary(asset, do_compile=bool(arguments.get("compile", False)))
        return text_result({
            "path": abs_path,
            "compile": compile_info,
            "message": "Shader asset updated.",
        })


def tool_inspect_current_graph(arguments):
    graph = Sh.Context.Graph
    if graph is None:
        raise ValueError("No graph is currently open.")

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
        for pin in node.InputPins:
            source = pin.SourceNode
            item["inputs"].append({
                "name": pin.Name,
                "sourceNodeId": source.Id if source is not None else None,
                "sourceNodeName": source.Name if source is not None else None,
                "sourceNodeType": type(source).__name__ if source is not None else None,
            })
        try:
            output_pins = node.OutputPins
        except AttributeError:
            output_pins = []
        for pin in output_pins:
            item["outputs"].append({
                "name": pin.Name,
                "direction": _pin_direction_name(getattr(pin, "Direction", None), "output"),
            })
        nodes.append(item)

    return text_result({
        "graph": {
            "id": graph.Id,
            "name": graph.Name,
            "type": type(graph).__name__,
        },
        "nodes": nodes,
    })
