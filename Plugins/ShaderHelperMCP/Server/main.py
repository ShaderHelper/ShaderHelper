import sys

try:
    from fastmcp import FastMCP
except ImportError:  # Compatibility with the official MCP Python SDK package.
    from mcp.server.fastmcp import FastMCP

from bridge import BridgeClient
from config import parse_args
from tools import register_tools


SERVER_NAME = "shaderhelper-mcp"


def create_server(argv: list[str] | None = None):
    config = parse_args(sys.argv[1:] if argv is None else argv)
    bridge = BridgeClient(config)
    mcp = FastMCP(
        SERVER_NAME,
        instructions=(
            "Start ShaderHelper with the ShaderHelperMCP plugin enabled first. "
            "Use create_shader_asset with asset_type='st_shader' plus create_shadertoy_graph for Shadertoy effects. "
            "Use create_shader_asset, create_material_asset, and create_render_graph for native HLSL/GLSL workflows. "
            "Use render_graph_feedback after creating a graph to inspect viewport output, shader logs, prints, and asserts."
        ),
    )
    register_tools(mcp, bridge)
    return mcp


def main(argv: list[str] | None = None):
    create_server(argv).run()


if __name__ == "__main__":
    main()
