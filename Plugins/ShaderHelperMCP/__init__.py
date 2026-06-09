import json
import os
import threading
import traceback
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse

import ShaderHelper as Sh


BRIDGE_PROTOCOL_VERSION = "0.1"
BRIDGE_NAME = "shaderhelper-mcp-bridge"
BRIDGE_VERSION = "0.1.0"
DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 11451
BRIDGE_PATH = "/bridge"

_server = globals().get("_server")
_server_thread = globals().get("_server_thread")


def json_text(data):
    return json.dumps(data, ensure_ascii=False, indent=2, sort_keys=True)


def text_result(data, is_error=False):
    if isinstance(data, str):
        text = data
        structured = None
    else:
        text = json_text(data)
        structured = data

    result = {
        "content": [{"type": "text", "text": text}],
        "isError": bool(is_error),
    }
    if structured is not None:
        result["structuredContent"] = structured
    return result


from .tool_registry import TOOLS, call_tool, tool_definitions, tool_names


def bridge_status():
    return {
        "name": BRIDGE_NAME,
        "version": BRIDGE_VERSION,
        "protocolVersion": BRIDGE_PROTOCOL_VERSION,
        "endpoint": f"http://{_server.server_address[0]}:{_server.server_address[1]}{BRIDGE_PATH}" if _server else None,
        "tools": tool_names(),
    }


def _handle_bridge_rpc(request):
    if not isinstance(request, dict):
        return {"jsonrpc": "2.0", "id": None, "error": {"code": -32600, "message": "Invalid Request"}}

    request_id = request.get("id")
    method = request.get("method")
    if method == "ping":
        return {"jsonrpc": "2.0", "id": request_id, "result": {}}
    if method == "bridge/status":
        return {"jsonrpc": "2.0", "id": request_id, "result": bridge_status()}
    if method == "tools/list":
        return {"jsonrpc": "2.0", "id": request_id, "result": {"tools": tool_definitions()}}
    if method == "tools/call":
        params = request.get("params") or {}
        name = params.get("name")
        arguments = params.get("arguments") or {}
        if name not in TOOLS:
            return {"jsonrpc": "2.0", "id": request_id, "error": {"code": -32602, "message": f"Unknown tool: {name}"}}
        try:
            result = Sh.Run(lambda: call_tool(name, arguments))
        except Exception as exc:
            print(f"ShaderHelperMCP bridge tool failed: {exc}")
            traceback.print_exc()
            result = text_result({"error": str(exc)}, is_error=True)
        return {"jsonrpc": "2.0", "id": request_id, "result": result}
    if method and method.startswith("notifications/"):
        return None

    return {
        "jsonrpc": "2.0",
        "id": request_id,
        "error": {
            "code": -32601,
            "message": f"Bridge method not found: {method}",
        },
    }


def _valid_origin(value):
    if not value:
        return True
    parsed = urlparse(value)
    return parsed.hostname in ("127.0.0.1", "localhost", "::1")


def _cors_origin(handler):
    origin = handler.headers.get("Origin")
    if origin and _valid_origin(origin):
        return origin
    return "http://localhost"


class _BridgeHandler(BaseHTTPRequestHandler):
    server_version = "ShaderHelperMCPBridge/0.1"

    def log_message(self, format, *args):
        return

    def _send_json(self, status, payload):
        body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Access-Control-Allow-Origin", _cors_origin(self))
        self.end_headers()
        self.wfile.write(body)

    def _send_empty(self, status):
        self.send_response(status)
        self.send_header("Content-Length", "0")
        self.end_headers()

    def do_OPTIONS(self):
        self.send_response(204)
        self.send_header("Access-Control-Allow-Origin", _cors_origin(self))
        self.send_header("Access-Control-Allow-Headers", "content-type, accept, mcp-protocol-version, mcp-session-id")
        self.send_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS, DELETE")
        self.send_header("Content-Length", "0")
        self.end_headers()

    def do_GET(self):
        self._send_empty(405)

    def do_DELETE(self):
        self._send_empty(405)

    def do_POST(self):
        parsed = urlparse(self.path)
        if parsed.path not in (BRIDGE_PATH, "/"):
            self._send_json(404, {"jsonrpc": "2.0", "error": {"code": -32000, "message": "ShaderHelper bridge endpoint not found"}})
            return

        if not _valid_origin(self.headers.get("Origin")):
            self._send_json(403, {"jsonrpc": "2.0", "error": {"code": -32000, "message": "Invalid Origin header"}})
            return

        try:
            length = int(self.headers.get("Content-Length", "0"))
            raw_body = self.rfile.read(length).decode("utf-8")
            request = json.loads(raw_body)
        except Exception:
            self._send_json(400, {"jsonrpc": "2.0", "error": {"code": -32700, "message": "Parse error"}})
            return

        if isinstance(request, list):
            response = [_handle_bridge_rpc(item) for item in request]
            response = [item for item in response if item is not None]
        else:
            response = _handle_bridge_rpc(request)

        if response is None or response == []:
            self._send_empty(202)
        else:
            self._send_json(200, response)


class _BridgeHTTPServer(ThreadingHTTPServer):
    allow_reuse_address = True


def start_bridge():
    global _server, _server_thread
    if _server is not None:
        return

    host = os.environ.get("SHADERHELPER_BRIDGE_HOST", DEFAULT_HOST)
    configured_port = os.environ.get("SHADERHELPER_BRIDGE_PORT")
    port = int(configured_port or DEFAULT_PORT)
    if host not in ("127.0.0.1", "localhost", "::1"):
        raise ValueError("ShaderHelperMCP only allows localhost binding.")

    _server = _BridgeHTTPServer((host, port), _BridgeHandler)

    _server_thread = threading.Thread(target=_server.serve_forever, name="ShaderHelperMCPBridge", daemon=True)
    _server_thread.start()
    actual_host, actual_port = _server.server_address
    print(f"ShaderHelper MCP bridge listening on http://{actual_host}:{actual_port}{BRIDGE_PATH}")


def stop_bridge():
    global _server, _server_thread
    if _server is None:
        return
    server = _server
    thread = _server_thread
    _server = None
    _server_thread = None
    server.shutdown()
    server.server_close()
    if thread is not None and thread.is_alive():
        thread.join(timeout=2.0)
    print("ShaderHelper MCP bridge stopped.")


def Register():
    start_bridge()


def Unregister():
    stop_bridge()
