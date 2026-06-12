# ShaderHelper MCP

```text
AI agent
  -> external MCP server over stdio
  -> local ShaderHelper bridge
  -> ShaderHelper Python plugin
```

## Setup

1. Enable the `ShaderHelperMCP` plugin in ShaderHelper.

   ShaderHelper should print:

   ```text
   ShaderHelper MCP bridge listening on http://127.0.0.1:11451/bridge
   ```

2. Install `uv` once.

3. Install the MCP server command from this repository:

   ```powershell
   cd <ShaderHelper repo>
   uv tool install --editable ./Plugins/ShaderHelperMCP/Server
   uv tool update-shell
   ```

   Restart your terminal, VS Code, or AI client after `uv tool update-shell` so `shaderhelper-mcp` is available on `PATH`.

4. Add the MCP server to your AI client config.

   For clients using `mcpServers`:

   ```json
   {
     "mcpServers": {
       "shaderhelper": {
         "command": "shaderhelper-mcp",
         "args": []
       }
     }
   }
   ```

5. Restart the AI client.

## VS Code

For VS Code/Copilot, put this in `.vscode/mcp.json` or the user MCP config:

```json
{
  "servers": {
    "shaderhelper": {
      "type": "stdio",
      "command": "shaderhelper-mcp",
      "args": []
    }
  }
}
```

## AI Skills

Agent skills are included with this plugin:

```text
Plugins/ShaderHelperMCP/skills/shaderhelper-shadertoy/
Plugins/ShaderHelperMCP/skills/shaderhelper-native-render/
```

For local auto-discovery, copy the needed skill folders into your AI client's skills
directory.

Invoke them as:

```text
$shaderhelper-shadertoy
$shaderhelper-native-render
```

Available MCP tools:

- `shaderhelper_status`
- `list_builtin_shadertoy_resources`
- `create_shader_asset`
- `create_shadertoy_graph`
- `compile_shader_asset`
- `create_material_asset`
- `create_render_graph`
- `render_graph_feedback`
- `read_shader_asset`
- `update_shader_asset`
- `inspect_current_graph`

## Notes

- Keep ShaderHelper open while using the MCP tools.
- For a custom port, set `SHADERHELPER_BRIDGE_PORT` before starting ShaderHelper and the AI client.

```powershell
$env:SHADERHELPER_BRIDGE_PORT = "11451"
```
