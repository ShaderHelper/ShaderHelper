from typing import Any

from bridge import BridgeClient


def register_tools(mcp, bridge: BridgeClient):
    @mcp.tool()
    def shaderhelper_status() -> Any:
        """Return ShaderHelper bridge status, active asset directory, and current graph summary."""
        return bridge.call_tool("shaderhelper_status", {})

    @mcp.tool()
    def list_builtin_shadertoy_resources() -> Any:
        """List built-in ShaderHelper Shadertoy texture, cubemap, and volume assets."""
        return bridge.call_tool("list_builtin_shadertoy_resources", {})

    @mcp.tool()
    def create_shader_asset(
        name: str,
        code: str,
        asset_type: str = "shader",
        target_dir: str | None = None,
        language: str = "HLSL",
        stages: list[str] | None = None,
        entry_points: dict[str, str] | None = None,
        stage_macros: dict[str, list[str]] | None = None,
        channel_slot_types: list[str] | None = None,
        compile: bool = False,
        overwrite: bool = False,
    ) -> Any:
        """Create a Shader, StShader, or ShaderHeader asset."""
        arguments = {
            "name": name,
            "asset_type": asset_type,
            "target_dir": target_dir,
            "language": language,
            "code": code,
            "stages": stages,
            "entry_points": entry_points,
            "stage_macros": stage_macros,
            "channel_slot_types": channel_slot_types,
            "compile": compile,
            "overwrite": overwrite,
        }
        return bridge.call_tool("create_shader_asset", {key: value for key, value in arguments.items() if value is not None})

    @mcp.tool()
    def create_shadertoy_graph(
        name: str,
        passes: list[dict[str, Any]],
        target_dir: str | None = None,
        channels: list[dict[str, Any]] | None = None,
        output_pass: str | None = None,
        flip_y: bool = True,
        overwrite: bool = False,
    ) -> Any:
        """Create a ShaderToy graph from existing StShader pass assets."""
        arguments = {
            "name": name,
            "target_dir": target_dir,
            "passes": passes,
            "channels": channels,
            "output_pass": output_pass,
            "flip_y": flip_y,
            "overwrite": overwrite,
        }
        return bridge.call_tool("create_shadertoy_graph", {key: value for key, value in arguments.items() if value is not None})

    @mcp.tool()
    def compile_shader_asset(path: str, compile: bool = True) -> Any:
        """Compile a ShaderAsset and return success, warnings, errors, and status."""
        return bridge.call_tool("compile_shader_asset", {"path": path, "compile": compile})

    @mcp.tool()
    def create_material_asset(
        name: str,
        target_dir: str | None = None,
        vertex_shader_path: str | None = None,
        pixel_shader_path: str | None = None,
        overwrite: bool = False,
    ) -> Any:
        """Create a native Material asset from vertex and pixel Shader assets."""
        arguments = {
            "name": name,
            "target_dir": target_dir,
            "vertex_shader_path": vertex_shader_path,
            "pixel_shader_path": pixel_shader_path,
            "overwrite": overwrite,
        }
        return bridge.call_tool("create_material_asset", {key: value for key, value in arguments.items() if value is not None})

    @mcp.tool()
    def create_render_graph(
        name: str,
        target_dir: str | None = None,
        scene_objects: list[dict[str, Any]] | None = None,
        nodes: list[dict[str, Any]] | None = None,
        links: list[dict[str, Any]] | None = None,
        overwrite: bool = False,
    ) -> Any:
        """Create a native Render graph with scene objects, texture, mesh pass, compute pass, output nodes, and directional pin links."""
        arguments = {
            "name": name,
            "target_dir": target_dir,
            "scene_objects": scene_objects,
            "nodes": nodes,
            "links": links,
            "overwrite": overwrite,
        }
        return bridge.call_tool("create_render_graph", {key: value for key, value in arguments.items() if value is not None})

    @mcp.tool()
    def render_graph_feedback(
        graph_path: str,
        frames: int = 1,
        time_step: float = 1.0 / 60.0,
        capture_viewport: bool = True,
        output_dir: str | None = None,
        overwrite: bool = False,
    ) -> Any:
        """Open a graph, render frames, capture viewport output, and return render diagnostics plus per-pass GPU times."""
        arguments = {
            "graph_path": graph_path,
            "frames": frames,
            "time_step": time_step,
            "capture_viewport": capture_viewport,
            "output_dir": output_dir,
            "overwrite": overwrite,
        }
        return bridge.call_tool("render_graph_feedback", {key: value for key, value in arguments.items() if value is not None})

    @mcp.tool()
    def read_shader_asset(path: str) -> Any:
        """Load a ShaderAsset and return its visible editor code."""
        return bridge.call_tool("read_shader_asset", {"path": path})

    @mcp.tool()
    def update_shader_asset(
        path: str,
        code: str,
        language: str | None = None,
        stages: list[str] | None = None,
        entry_points: dict[str, str] | None = None,
        stage_macros: dict[str, list[str]] | None = None,
        channel_slot_types: list[str] | None = None,
        compile: bool = False,
    ) -> Any:
        """Replace a ShaderAsset's visible editor code, save it through the AssetObject save path, and optionally compile it."""
        arguments = {
            "path": path,
            "code": code,
            "language": language,
            "stages": stages,
            "entry_points": entry_points,
            "stage_macros": stage_macros,
            "channel_slot_types": channel_slot_types,
            "compile": compile,
        }
        return bridge.call_tool("update_shader_asset", {key: value for key, value in arguments.items() if value is not None})

    @mcp.tool()
    def inspect_current_graph() -> Any:
        """Return the currently open ShaderHelper graph nodes, input/output pins, links, and Shadertoy pass code."""
        return bridge.call_tool("inspect_current_graph", {})
