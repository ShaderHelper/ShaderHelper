---
name: shaderhelper-shadertoy
description: Create, wire, inspect, and debug Shadertoy-style ShaderHelper effects through the local shaderhelper MCP server. Use when an AI agent needs ShaderHelper Shadertoy passes, StShader assets, mainImage GLSL/HLSL code, iChannel textures/cubemaps/volumes, multipass Shadertoy graphs, previous-frame feedback, built-in Shadertoy resources, or ShaderToy graph inspection.
---

# ShaderHelper Shadertoy

## Workflow

Always start with `shaderhelper_status` to verify the bridge, active asset browser directory, open graph, and available tools.

1. Create one or more pass shaders with `create_shader_asset` using `asset_type: "st_shader"`.
2. Use `list_builtin_shadertoy_resources` before referencing built-in texture, cubemap, or volume names.
3. Connect passes and channels with `create_shadertoy_graph`.
4. Compile created or updated pass shaders when validation matters.
5. Use `inspect_current_graph` when debugging the currently open ShaderToy graph.
6. Report created asset paths, graph path, output pass, and compile status.

`target_dir` must stay inside ShaderHelper's current asset browser directory. Prefer a relative `target_dir` for generated Shadertoy assets. Leave `overwrite` as `false` unless the user asked to replace an asset.

## Pass Assets

Use `create_shader_asset` with `asset_type: "st_shader"` for Shadertoy-style pass code, usually GLSL with:

```glsl
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = fragCoord / iResolution.xy;
    fragColor = vec4(uv, 0.0, 1.0);
}
```

Example call shape:

```json
{
  "name": "RippleImage",
  "asset_type": "st_shader",
  "language": "GLSL",
  "code": "void mainImage(out vec4 fragColor, in vec2 fragCoord) { ... }",
  "channel_slot_types": ["Texture2D", "TextureCube", "Texture3D"],
  "compile": true
}
```

`channel_slot_types` accepts `Texture2D`, `TextureCube`, and `Texture3D`. Omitted slots default to `Texture2D`.

## Graphs And Channels

Use pass paths returned by `create_shader_asset`.

```json
{
  "name": "RippleToy",
  "passes": [
    {"name": "Image", "path": "D:/.../RippleImage.stshader"}
  ],
  "channels": [
    {
      "pass": "Image",
      "index": 0,
      "builtin": {"kind": "texture", "name": "Noise"},
      "filter": "linear",
      "wrap": "repeat"
    }
  ],
  "output_pass": "Image",
  "flip_y": true
}
```

Channel sources may use:

- `builtin`: `{ "kind": "texture" | "cubemap" | "volume", "name": "..." }`
- `asset_path`: path to a texture asset
- `source_pass`: name of another pass in the graph

For feedback, set `previous_frame: true`; self-sourcing a pass is treated as previous-frame feedback.

## Debugging

For existing pass edits, use `read_shader_asset`, then `update_shader_asset` with `compile: true`. If wiring is suspect, call `inspect_current_graph` to see nodes, channel inputs, and Shadertoy pass code.

Native ShaderHelper shader-side `Print`, `PrintAtMouse`, and `Assert` helpers can be used where supported by the running shader path. Add focused probes, compile, render, then inspect ShaderHelper logs and screenshots.

## Tool Guide

- `shaderhelper_status`: check bridge health, active asset directory, current graph, and tools.
- `list_builtin_shadertoy_resources`: discover built-in Shadertoy channel resources.
- `create_shader_asset`: create one StShader pass with `asset_type: "st_shader"`.
- `create_shadertoy_graph`: wire passes, built-ins, asset textures, pass outputs, feedback, and output.
- `compile_shader_asset`: compile a pass shader and read compile status.
- `read_shader_asset`: load visible editor code from an existing pass.
- `update_shader_asset`: replace pass code, update channel slot types, and optionally compile.
- `inspect_current_graph`: inspect the currently open graph's nodes, links, and Shadertoy code.
