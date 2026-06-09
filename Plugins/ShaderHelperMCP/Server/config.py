import argparse
import os
from dataclasses import dataclass


DEFAULT_BRIDGE_HOST = "127.0.0.1"
DEFAULT_BRIDGE_PORT = 11451
DEFAULT_BRIDGE_PATH = "/bridge"
DEFAULT_REQUEST_TIMEOUT_SECONDS = 120.0


@dataclass
class ServerConfig:
    bridge_url: str | None
    bridge_host: str
    bridge_path: str
    bridge_port: int
    request_timeout: float


def _normalize_bridge_url(value: str | None) -> str | None:
    value = str(value or "").strip()
    if not value:
        return None
    return value.rstrip("/")


def _env_int(name: str, default: int | None = None) -> int | None:
    value = os.environ.get(name)
    if value is None or value == "":
        return default
    return int(value)


def _env_float(name: str, default: float) -> float:
    value = os.environ.get(name)
    if value is None or value == "":
        return default
    return float(value)


def parse_args(argv: list[str]) -> ServerConfig:
    parser = argparse.ArgumentParser(description="FastMCP server for ShaderHelper.")
    parser.add_argument("--bridge-url", default=os.environ.get("SHADERHELPER_BRIDGE_URL"))
    parser.add_argument("--bridge-host", default=os.environ.get("SHADERHELPER_BRIDGE_HOST", DEFAULT_BRIDGE_HOST))
    parser.add_argument("--bridge-path", default=os.environ.get("SHADERHELPER_BRIDGE_PATH", DEFAULT_BRIDGE_PATH))
    parser.add_argument("--request-timeout", type=float, default=_env_float("SHADERHELPER_BRIDGE_REQUEST_TIMEOUT", DEFAULT_REQUEST_TIMEOUT_SECONDS))
    args = parser.parse_args(argv)

    configured_port = os.environ.get("SHADERHELPER_BRIDGE_PORT")
    bridge_port = int(configured_port or DEFAULT_BRIDGE_PORT)

    return ServerConfig(
        bridge_url=_normalize_bridge_url(args.bridge_url),
        bridge_host=args.bridge_host,
        bridge_path=args.bridge_path,
        bridge_port=bridge_port,
        request_timeout=args.request_timeout,
    )
