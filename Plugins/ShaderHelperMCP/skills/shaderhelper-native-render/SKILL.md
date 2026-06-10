---
name: shaderhelper-native-render
description: Create, compile, inspect, and debug native ShaderHelper HLSL/GLSL shader, material, scene object, mesh pass, and render graph assets through the local shaderhelper MCP server. Use when an AI agent needs native .shader assets, ShaderHeader assets, vertex/pixel/compute stages, entry points, stage macros, .material assets, .render graphs, mesh/camera scene objects, mesh pass nodes, compute pass nodes, render output wiring, or shader-side Print/PrintAtMouse/Assert debugging.
---

# ShaderHelper Native Render

## Workflow

Always start with `shaderhelper_status` to verify the bridge, active asset browser directory, open graph, and available tools.

1. Create native `.shader` or ShaderHeader assets with `create_shader_asset`.
2. Compile shaders with `compile_shader_asset`, or set `compile: true` while creating/updating.
3. Create a `.material` with `create_material_asset` when vertex/pixel shader binding is needed.
4. Create a `.render` graph with `create_render_graph` for scene objects, texture, mesh, compute, and output nodes.
5. Use `render_graph_feedback` to open the graph, render frames, capture the viewport, inspect logs, and read per-pass GPU times.
6. Use `read_shader_asset`, `update_shader_asset`, and `inspect_current_graph` while iterating.
7. Report created asset paths, graph path, viewport capture path, enabled stages, and compile/render status.

`target_dir` must stay inside ShaderHelper's current asset browser directory. Prefer a relative `target_dir` for generated native assets. Leave `overwrite` as `false` unless the user asked to replace an asset.

## Shader Assets

Use `create_shader_asset` with `asset_type: "shader"` for native Shader assets and `asset_type: "shader_header"` for reusable headers. `language` may be `HLSL` or `GLSL`.

Defaults:

- `stages` defaults to `["vertex", "pixel"]`.
- HLSL default entry points are `MainVS`, `MainPS`, and `MainCS`.
- GLSL default entry point is `main`.
- GLSL stage macros default to the upper-case stage name.

Example call shape:

```json
{
  "name": "LitMesh",
  "language": "HLSL",
  "stages": ["vertex", "pixel"],
  "entry_points": {"vertex": "MainVS", "pixel": "MainPS"},
  "code": "...",
  "compile": true
}
```

For compute shaders, set `stages: ["compute"]` and provide the compute entry point when it differs from `MainCS`.

## Materials

Use `create_material_asset` when a material should bind vertex and/or pixel shader assets.

```json
{
  "name": "LitMeshMaterial",
  "vertex_shader_path": "D:/.../LitMesh.shader",
  "pixel_shader_path": "D:/.../LitMesh.shader"
}
```

## Render Graphs

Use `create_render_graph` for native `.render` graph assets. Supported node types include `texture`, `texture2d`, `texture_cube`, `texture3d`, `mesh`, `mesh_pass`, `compute`, `compute_pass`, `output`, and `render_output`. The output node is available as `Output`.

Use `scene_objects` when a mesh pass should draw actual geometry. Supported scene object types are `scene`, `mesh`, and `camera`. Mesh scene objects can set `model_path`, `vertex_count`, and `instance_count`; camera scene objects can set transform and camera projection fields. Mesh pass node specs can set `camera` to a camera scene object id/name and `meshes` to entries such as `{ "scene_object": "Mesh", "material_path": "D:/.../Material.material" }`.

```json
{
  "name": "LitMeshRender",
  "scene_objects": [
    {"id": "Camera", "type": "camera", "position": [0, 0, -5], "rotation": [0, 0, 0], "preview": true},
    {"id": "MeshObject", "type": "mesh", "model_path": "D:/.../Sphere.model"}
  ],
  "nodes": [
    {
      "name": "Mesh",
      "type": "mesh",
      "rt_size": [1024, 1024],
      "color_target_count": 1,
      "camera": "Camera",
      "meshes": [{"scene_object": "MeshObject", "material_path": "D:/.../LitMeshMaterial.material"}]
    },
    {"name": "Compute", "type": "compute", "shader_path": "D:/.../Compute.shader", "thread_group_count": [16, 16, 1]}
  ],
  "links": [
    {"from": "Mesh", "out_pin": "Color0", "from_direction": "output", "to": "Output", "in_pin": "RT", "to_direction": "input"}
  ]
}
```

Use `asset_path` for texture nodes and `shader_path` for compute nodes. Mesh pass color output pins are `Color0`, `Color1`, and so on; render output input is `RT`. Link pins with `{ "from", "out_pin", "from_direction", "to", "in_pin", "to_direction" }`; omitted pins default to `RT`, `output`, and `input`. Specify direction when a node has same-named input and output pins, such as mesh pass color pins or compute `RWTexture2D` bindings.

## Debugging

Native Shader assets can use ShaderHelper shader-side `Print`, `PrintAtMouse`, and `Assert` helpers. Add focused probes, compile, then call `render_graph_feedback` to render, capture the viewport, inspect print/assert/error logs, and compare `passTimes`.

When a shader fails:

1. Read current code with `read_shader_asset` if needed.
2. Update with the smallest code change using `update_shader_asset`.
3. Compile with `compile_shader_asset` or `update_shader_asset` using `compile: true`.
4. Call `render_graph_feedback` on the graph and compare the viewport image/logs against the target.
5. If wiring or graph state is suspect, call `inspect_current_graph`.

## Tool Guide

- `shaderhelper_status`: check bridge health, active asset directory, current graph, and tools.
- `create_shader_asset`: create native Shader or ShaderHeader assets.
- `compile_shader_asset`: compile a ShaderAsset and return enabled stages, warnings, errors, and status.
- `create_material_asset`: create a Material from vertex and pixel Shader assets.
- `create_render_graph`: create a Render graph with scene objects, texture, mesh, compute, and output nodes.
- `render_graph_feedback`: open a graph, render frames, capture the viewport image, and return render diagnostics plus `passTimes`.
- `read_shader_asset`: load visible editor code and compile metadata.
- `update_shader_asset`: replace shader code, update language/stages/entry points/macros, and optionally compile.
- `inspect_current_graph`: inspect the currently open graph's nodes, input/output pins, and links.
