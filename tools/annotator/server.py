#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import mimetypes
import os
import sys
import threading
import time
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any
from urllib.parse import parse_qs, unquote, urlparse


TOOLS_DIR = Path(__file__).resolve().parent
PROJECT_DIR = TOOLS_DIR.parent.parent
PRE_DIR = PROJECT_DIR / "assets" / "pre"
ANNOTATIONS_DIR = PRE_DIR / "annotations"
STATIC_FILES = {
    "/": TOOLS_DIR / "index.html",
    "/app.js": TOOLS_DIR / "app.js",
    "/annotator_annotations.js": TOOLS_DIR / "annotator_annotations.js",
    "/annotator_common.js": TOOLS_DIR / "annotator_common.js",
    "/annotator_gallery.js": TOOLS_DIR / "annotator_gallery.js",
    "/styles.css": TOOLS_DIR / "styles.css",
}
HOT_RELOAD_FILES = sorted({Path(__file__).resolve(), *STATIC_FILES.values()}, key=lambda path: str(path))
HOT_RELOAD_INTERVAL_SECONDS = 0.75


def list_sheet_files() -> list[Path]:
    return sorted(path for path in PRE_DIR.glob("*.png") if path.is_file())


def annotation_path_for_sheet(sheet_name: str) -> Path:
    sheet_path = PRE_DIR / sheet_name
    return ANNOTATIONS_DIR / f"{sheet_path.stem}.annotations.json"


def load_annotations(sheet_name: str) -> dict[str, Any]:
    annotation_path = annotation_path_for_sheet(sheet_name)
    if not annotation_path.exists():
        return {"image": sheet_name, "annotations": []}

    try:
        return json.loads(annotation_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError:
        return {"image": sheet_name, "annotations": []}


def save_annotations(sheet_name: str, payload: dict[str, Any]) -> None:
    ANNOTATIONS_DIR.mkdir(parents=True, exist_ok=True)
    annotation_path = annotation_path_for_sheet(sheet_name)
    temp_path = annotation_path.with_suffix(annotation_path.suffix + ".tmp")
    temp_path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    os.replace(temp_path, annotation_path)


def file_mtime_ns(path: Path) -> int:
    try:
        return path.stat().st_mtime_ns
    except FileNotFoundError:
        return 0


def hot_reload_revision() -> str:
    return "|".join(f"{path.name}:{file_mtime_ns(path)}" for path in HOT_RELOAD_FILES)


def start_hot_reload_watcher(server: ThreadingHTTPServer) -> threading.Thread:
    baseline = {path: file_mtime_ns(path) for path in HOT_RELOAD_FILES}

    def watch() -> None:
        while not getattr(server, "_stop_hot_reload", False):
            time.sleep(HOT_RELOAD_INTERVAL_SECONDS)
            for path in HOT_RELOAD_FILES:
                if file_mtime_ns(path) != baseline.get(path, 0):
                    print(f"Hot reload: detected change in {path.name}")
                    server._should_restart = True
                    server.shutdown()
                    return

    thread = threading.Thread(target=watch, name="annotator-hot-reload", daemon=True)
    thread.start()
    return thread


class AnnotatorHandler(BaseHTTPRequestHandler):
    server_version = "z1m-annotator/0.1"

    def do_GET(self) -> None:
        parsed = urlparse(self.path)
        if parsed.path == "/api/sheets":
            self._handle_list_sheets()
            return

        if parsed.path == "/api/annotations":
            self._handle_get_annotations(parsed)
            return

        if parsed.path == "/api/project_annotations":
            self._handle_project_annotations()
            return

        if parsed.path == "/api/dev_revision":
            self._send_json({"revision": hot_reload_revision()})
            return

        if parsed.path.startswith("/pre/"):
            self._serve_pre_asset(parsed.path)
            return

        static_path = STATIC_FILES.get(parsed.path)
        if static_path is not None:
            self._serve_file(static_path)
            return

        self.send_error(HTTPStatus.NOT_FOUND, "Not found")

    def do_POST(self) -> None:
        parsed = urlparse(self.path)
        if parsed.path != "/api/annotations":
            self.send_error(HTTPStatus.NOT_FOUND, "Not found")
            return

        query = parse_qs(parsed.query)
        sheet_name = query.get("sheet", [""])[0]
        if not sheet_name:
            self.send_error(HTTPStatus.BAD_REQUEST, "Missing sheet parameter")
            return

        if not (PRE_DIR / sheet_name).is_file():
            self.send_error(HTTPStatus.NOT_FOUND, "Unknown sheet")
            return

        content_length = int(self.headers.get("Content-Length", "0"))
        body = self.rfile.read(content_length)
        try:
            payload = json.loads(body.decode("utf-8"))
        except json.JSONDecodeError:
            self.send_error(HTTPStatus.BAD_REQUEST, "Invalid JSON")
            return

        if not isinstance(payload, dict) or "annotations" not in payload:
            self.send_error(HTTPStatus.BAD_REQUEST, "Malformed annotation payload")
            return

        payload["image"] = sheet_name
        save_annotations(sheet_name, payload)
        self._send_json({"ok": True})

    def log_message(self, fmt: str, *args: Any) -> None:
        print(f"{self.address_string()} - {fmt % args}")

    def _handle_list_sheets(self) -> None:
        manifest_path = PRE_DIR / "manifest.json"
        manifest_assets: dict[str, dict[str, Any]] = {}
        if manifest_path.exists():
            try:
                manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
                for asset in manifest.get("assets", []):
                    file_name = asset.get("file")
                    if isinstance(file_name, str):
                        manifest_assets[file_name] = asset
            except json.JSONDecodeError:
                pass

        sheets = []
        for path in list_sheet_files():
            asset = manifest_assets.get(path.name, {})
            sheets.append(
                {
                    "file": path.name,
                    "name": asset.get("name", path.stem),
                    "image_url": f"/pre/{path.name}",
                    "annotation_url": f"/api/annotations?sheet={path.name}",
                    "annotation_file": str(annotation_path_for_sheet(path.name).relative_to(PROJECT_DIR)),
                    "asset_page": asset.get("asset_page", ""),
                    "download_url": asset.get("download_url", ""),
                }
            )

        self._send_json({"sheets": sheets})

    def _handle_project_annotations(self) -> None:
        manifest_path = PRE_DIR / "manifest.json"
        manifest_assets: dict[str, dict[str, Any]] = {}
        if manifest_path.exists():
            try:
                manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
                for asset in manifest.get("assets", []):
                    file_name = asset.get("file")
                    if isinstance(file_name, str):
                        manifest_assets[file_name] = asset
            except json.JSONDecodeError:
                pass

        sheets = []
        for path in list_sheet_files():
            asset = manifest_assets.get(path.name, {})
            payload = load_annotations(path.name)
            sheets.append(
                {
                    "file": path.name,
                    "name": asset.get("name", path.stem),
                    "image_url": f"/pre/{path.name}",
                    "annotation_url": f"/api/annotations?sheet={path.name}",
                    "annotation_file": str(annotation_path_for_sheet(path.name).relative_to(PROJECT_DIR)),
                    "asset_page": asset.get("asset_page", ""),
                    "download_url": asset.get("download_url", ""),
                    "annotations": payload.get("annotations", []),
                }
            )

        self._send_json({"sheets": sheets})

    def _handle_get_annotations(self, parsed: Any) -> None:
        query = parse_qs(parsed.query)
        sheet_name = query.get("sheet", [""])[0]
        if not sheet_name:
            self.send_error(HTTPStatus.BAD_REQUEST, "Missing sheet parameter")
            return

        if not (PRE_DIR / sheet_name).is_file():
            self.send_error(HTTPStatus.NOT_FOUND, "Unknown sheet")
            return

        self._send_json(load_annotations(sheet_name))

    def _serve_pre_asset(self, request_path: str) -> None:
        relative = request_path.removeprefix("/pre/")
        target = PRE_DIR / unquote(relative)
        if not target.is_file():
            self.send_error(HTTPStatus.NOT_FOUND, "Asset not found")
            return

        self._serve_file(target)

    def _serve_file(self, path: Path) -> None:
        if not path.is_file():
            self.send_error(HTTPStatus.NOT_FOUND, "Not found")
            return

        content_type = mimetypes.guess_type(path.name)[0] or "application/octet-stream"
        data = path.read_bytes()
        self.send_response(HTTPStatus.OK)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def _send_json(self, payload: dict[str, Any]) -> None:
        data = json.dumps(payload).encode("utf-8")
        self.send_response(HTTPStatus.OK)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8765)
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    ANNOTATIONS_DIR.mkdir(parents=True, exist_ok=True)
    server = ThreadingHTTPServer((args.host, args.port), AnnotatorHandler)
    server._should_restart = False
    server._stop_hot_reload = False
    watcher = start_hot_reload_watcher(server)
    print(f"Annotator running at http://{args.host}:{args.port}")
    print(f"Sheets: {PRE_DIR}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server._stop_hot_reload = True
        server.server_close()
        watcher.join(timeout=1.0)

    if getattr(server, "_should_restart", False):
        os.execv(sys.executable, [sys.executable, *sys.argv])


if __name__ == "__main__":
    main()
