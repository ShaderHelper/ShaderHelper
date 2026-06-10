from .shader_tools import (
    tool_compile_shader_asset,
    tool_create_material_asset,
    tool_create_render_graph,
    tool_create_shader_asset,
    tool_create_shadertoy_graph,
    tool_inspect_current_graph,
    tool_list_builtin_shadertoy_resources,
    tool_read_shader_asset,
    tool_render_graph_feedback,
    tool_shaderhelper_status,
    tool_update_shader_asset,
)


TOOLS = {
    "shaderhelper_status": {
        "handler": tool_shaderhelper_status,
        "title": "ShaderHelper MCP status",
        "description": "Return ShaderHelper bridge status, active asset directory, and current graph summary.",
        "inputSchema": {"type": "object", "properties": {}, "additionalProperties": False},
    },
    "list_builtin_shadertoy_resources": {
        "handler": tool_list_builtin_shadertoy_resources,
        "title": "List built-in Shadertoy resources",
        "description": "List built-in ShaderHelper Shadertoy texture, cubemap, and volume assets that can be connected to iChannel inputs.",
        "inputSchema": {"type": "object", "properties": {}, "additionalProperties": False},
    },
    "create_shader_asset": {
        "handler": tool_create_shader_asset,
        "title": "Create shader asset",
        "description": "Create a Shader, StShader, or ShaderHeader asset. Native shaders may be HLSL or GLSL and can be compiled immediately.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "name": {"type": "string"},
                "asset_type": {"type": "string", "enum": ["shader", "st_shader", "shader_header"], "default": "shader"},
                "target_dir": {"type": "string"},
                "language": {"type": "string", "enum": ["HLSL", "GLSL"], "default": "HLSL"},
                "code": {"type": "string"},
                "stages": {"type": "array", "items": {"type": "string", "enum": ["vertex", "pixel", "compute"]}},
                "entry_points": {"type": "object"},
                "stage_macros": {"type": "object"},
                "channel_slot_types": {
                    "type": "array",
                    "items": {"type": "string", "enum": ["Texture2D", "TextureCube", "Texture3D"]},
                },
                "compile": {"type": "boolean", "default": False},
                "overwrite": {"type": "boolean", "default": False},
            },
            "required": ["name", "code"],
        },
    },
    "create_shadertoy_graph": {
        "handler": tool_create_shadertoy_graph,
        "title": "Create Shadertoy graph",
        "description": "Create a ShaderToy graph from existing StShader pass assets and optional channel links.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "name": {"type": "string"},
                "target_dir": {"type": "string"},
                "passes": {
                    "type": "array",
                    "items": {"type": "object", "description": "{name, path}"},
                },
                "channels": {
                    "type": "array",
                    "items": {"type": "object", "description": "{pass,index,source_pass|asset_path|builtin,filter,wrap,previous_frame}"},
                },
                "output_pass": {"type": "string"},
                "flip_y": {"type": "boolean", "default": True},
                "overwrite": {"type": "boolean", "default": False},
            },
            "required": ["name", "passes"],
        },
    },
    "compile_shader_asset": {
        "handler": tool_compile_shader_asset,
        "title": "Compile shader asset",
        "description": "Compile a ShaderAsset and return success, warnings, errors, enabled stages, and current status.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "path": {"type": "string"},
                "compile": {"type": "boolean", "default": True},
            },
            "required": ["path"],
        },
    },
    "create_material_asset": {
        "handler": tool_create_material_asset,
        "title": "Create material asset",
        "description": "Create a native Material asset from vertex and pixel Shader assets.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "name": {"type": "string"},
                "target_dir": {"type": "string"},
                "vertex_shader_path": {"type": "string"},
                "pixel_shader_path": {"type": "string"},
                "overwrite": {"type": "boolean", "default": False},
            },
            "required": ["name"],
        },
    },
    "create_render_graph": {
        "handler": tool_create_render_graph,
        "title": "Create native render graph",
        "description": "Create a Render graph asset with scene objects, texture nodes, mesh pass nodes, compute pass nodes, output nodes, and pin links.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "name": {"type": "string"},
                "target_dir": {"type": "string"},
                "scene_objects": {
                    "type": "array",
                    "items": {
                        "type": "object",
                        "description": "{id,name,type,parent,position,rotation,scale,model_path,vertex_count,instance_count,orthographic,ortho_size,vertical_fov,near_plane,far_plane,preview}",
                    },
                },
                "nodes": {
                    "type": "array",
                    "items": {
                        "type": "object",
                        "description": "{name,type,asset_path|shader_path,rt_size,color_target_count,depth_enabled,thread_group_count,camera,meshes}; mesh pass meshes use {scene_object,material_path|material_paths}",
                    },
                },
                "links": {
                    "type": "array",
                    "items": {"type": "object", "description": "{from,out_pin,from_direction,to,in_pin,to_direction}; directions default to output/input"},
                },
                "overwrite": {"type": "boolean", "default": False},
            },
            "required": ["name"],
        },
    },
    "render_graph_feedback": {
        "handler": tool_render_graph_feedback,
        "title": "Render graph feedback",
        "description": "Open a Render or ShaderToy graph, render frames, capture the viewport image, and return render logs plus per-pass GPU times.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "graph_path": {"type": "string"},
                "frames": {"type": "integer", "default": 1},
                "time_step": {"type": "number", "default": 0.016666667},
                "capture_viewport": {"type": "boolean", "default": True},
                "output_dir": {"type": "string", "description": "Relative to ShaderHelper's saved capture directory."},
                "overwrite": {"type": "boolean", "default": False},
            },
            "required": ["graph_path"],
        },
    },
    "read_shader_asset": {
        "handler": tool_read_shader_asset,
        "title": "Read shader asset",
        "description": "Load a ShaderAsset and return its visible editor code.",
        "inputSchema": {
            "type": "object",
            "properties": {"path": {"type": "string"}},
            "required": ["path"],
        },
    },
    "update_shader_asset": {
        "handler": tool_update_shader_asset,
        "title": "Update shader asset",
        "description": "Replace a ShaderAsset's visible editor code, save it through the AssetObject save path, and optionally compile it.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "path": {"type": "string"},
                "code": {"type": "string"},
                "language": {"type": "string", "enum": ["GLSL", "HLSL"]},
                "stages": {"type": "array", "items": {"type": "string", "enum": ["vertex", "pixel", "compute"]}},
                "entry_points": {"type": "object"},
                "stage_macros": {"type": "object"},
                "channel_slot_types": {
                    "type": "array",
                    "items": {"type": "string", "enum": ["Texture2D", "TextureCube", "Texture3D"]},
                },
                "compile": {"type": "boolean", "default": False},
            },
            "required": ["path", "code"],
        },
    },
    "inspect_current_graph": {
        "handler": tool_inspect_current_graph,
        "title": "Inspect current graph",
        "description": "Return the currently open ShaderHelper graph nodes, input/output pins, links, and Shadertoy pass code.",
        "inputSchema": {"type": "object", "properties": {}, "additionalProperties": False},
    },
}


def tool_names():
    return sorted(TOOLS.keys())


def tool_definitions():
    result = []
    for name in tool_names():
        tool = TOOLS[name]
        result.append({
            "name": name,
            "title": tool["title"],
            "description": tool["description"],
            "inputSchema": tool["inputSchema"],
        })
    return result


def call_tool(name, arguments):
    return TOOLS[name]["handler"](arguments or {})
