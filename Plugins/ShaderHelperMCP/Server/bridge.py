import itertools
import json
import urllib.error
import urllib.request
from typing import Any

from config import ServerConfig


class BridgeError(RuntimeError):
    pass


class BridgeClient:
    def __init__(self, config: ServerConfig):
        self.config = config
        self._request_ids = itertools.count(1)
        self._opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))

    def urls(self) -> list[str]:
        if self.config.bridge_url:
            return [self.config.bridge_url]

        path = self.config.bridge_path
        if not path.startswith("/"):
            path = "/" + path
        return [f"http://{self.config.bridge_host}:{self.config.bridge_port}{path}"]

    def call(self, method: str, params: dict[str, Any] | None = None, timeout: float | None = None) -> Any:
        payload = {
            "jsonrpc": "2.0",
            "id": next(self._request_ids),
            "method": method,
        }
        if params is not None:
            payload["params"] = params

        body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        timeout = self.config.request_timeout if timeout is None else timeout
        errors = []

        for url in self.urls():
            request = urllib.request.Request(
                url,
                data=body,
                headers={"Content-Type": "application/json"},
                method="POST",
            )
            try:
                with self._opener.open(request, timeout=timeout) as response:
                    raw = response.read().decode("utf-8")
            except (OSError, urllib.error.URLError, urllib.error.HTTPError) as exc:
                errors.append(f"{url}: {exc}")
                continue

            try:
                message = json.loads(raw)
            except json.JSONDecodeError as exc:
                raise BridgeError(f"Bridge returned invalid JSON from {url}: {exc}") from exc

            if "error" in message:
                error = message["error"]
                raise BridgeError(error.get("message", str(error)))
            return message.get("result")

        detail = "; ".join(errors) if errors else "no bridge URL configured"
        raise BridgeError(f"Cannot connect to ShaderHelper bridge. {detail}")

    def call_tool(self, name: str, arguments: dict[str, Any]) -> Any:
        result = self.call("tools/call", {"name": name, "arguments": arguments})
        if not isinstance(result, dict):
            return result

        content = result.get("content") or []
        text = "\n".join(str(item.get("text", "")) for item in content if isinstance(item, dict) and item.get("type") == "text")
        if result.get("isError"):
            raise BridgeError(text or f"ShaderHelper tool failed: {name}")

        if "structuredContent" in result:
            return result["structuredContent"]
        return text
