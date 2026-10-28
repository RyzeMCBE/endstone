#!/usr/bin/env python3
"""Export vanilla registries from official BDS 1.26.60 with Endstone on Linux."""
from __future__ import annotations

import hashlib
import json
import os
import queue
import shutil
import subprocess
import sys
import threading
import time
from datetime import datetime, timezone
from pathlib import Path

REQUIRED = (
    "biomes.json", "block_palette.nbt", "block_states.json", "block_tags.json",
    "block_types.json", "creative_groups.json", "creative_items.nbt",
    "item_components.nbt", "item_tags.json", "items.json", "recipes.json",
)


def main() -> int:
    root = Path.cwd()
    server_dir = root / "bds-runtime"
    output = root / "export-output"
    output.mkdir(exist_ok=True)
    # Never launch Python from the repository root: its endstone/ source tree
    # would shadow the just-installed wheel (missing compiled endstone._python).
    runner_cwd = root / "bds-runner"
    runner_cwd.mkdir(exist_ok=True)
    lines: queue.Queue[str] = queue.Queue()
    started = exported = False
    errors: list[str] = []
    version = None
    proc: subprocess.Popen[str] | None = None

    def read_output(process: subprocess.Popen[str]) -> None:
        assert process.stdout is not None
        with (output / "server.log").open("w", encoding="utf-8", errors="replace") as log:
            for line in process.stdout:
                log.write(line)
                log.flush()
                print(line, end="", flush=True)
                lines.put(line)

    def await_log(marker: str, seconds: int) -> None:
        deadline = time.monotonic() + seconds
        while time.monotonic() < deadline:
            try:
                line = lines.get(timeout=1)
            except queue.Empty:
                if proc is not None and proc.poll() is not None:
                    raise RuntimeError("BDS exited with code " + str(proc.returncode))
                continue
            if "Vanilla data export failed:" in line:
                raise RuntimeError(line.strip())
            if marker in line:
                return
        raise TimeoutError("Timeout waiting for log marker: " + marker)

    def command(value: str) -> None:
        assert proc is not None and proc.stdin is not None
        proc.stdin.write(value + "\n")
        proc.stdin.flush()
        print("[export] Console command: " + value, flush=True)

    try:
        env = os.environ.copy()
        env["PYTHONUNBUFFERED"] = "1"
        print("[export] Starting official BDS through Endstone with SHA256-checked download", flush=True)
        proc = subprocess.Popen(
            [sys.executable, "-u", "-m", "endstone", "--yes",
             "--no-interactive", "--server-folder", str(server_dir)],
            cwd=runner_cwd, env=env, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, bufsize=1, text=True, encoding="utf-8",
            errors="replace",
        )
        threading.Thread(target=read_output, args=(proc,), daemon=True).start()
        await_log("Server started", 600)
        started = True
        path = server_dir / "version.txt"
        if not path.is_file():
            raise RuntimeError("BDS version.txt was not created")
        version = path.read_text(encoding="utf-8").strip()
        if version != "1.26.60":
            raise RuntimeError("Unexpected BDS version " + repr(version))
        command("exportvanilla")
        await_log("Vanilla data export completed:", 600)
        exported = True
    except (OSError, RuntimeError, TimeoutError) as exc:
        errors.append(str(exc))
        print("::error::" + str(exc), flush=True)
    finally:
        if proc is not None and proc.poll() is None:
            try:
                command("stop")
                proc.wait(timeout=45)
            except (OSError, subprocess.TimeoutExpired):
                proc.terminate()
                try:
                    proc.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    proc.kill()
                    proc.wait()
        source = server_dir / "data"
        if source.is_dir():
            shutil.copytree(source, output / "data", dirs_exist_ok=True)

    files: dict[str, dict[str, object]] = {}
    missing = []
    for name in REQUIRED:
        path = output / "data" / name
        if not path.is_file() or path.stat().st_size == 0:
            missing.append(name)
            continue
        contents = path.read_bytes()
        files[name] = {
            "bytes": len(contents),
            "sha256": hashlib.sha256(contents).hexdigest(),
        }
    if missing:
        errors.append("Missing or empty files: " + ", ".join(missing))

    counts: dict[str, int] = {}
    if not missing:
        for name in ("block_types", "block_states", "items", "biomes"):
            try:
                contents = json.loads((output / "data" / (name + ".json")).read_text(encoding="utf-8"))
                counts[name] = len(contents)
            except (OSError, ValueError, TypeError) as exc:
                errors.append("Invalid " + name + ".json: " + str(exc))
        if counts.get("block_types", 0) < 1000 or counts.get("block_states", 0) < 10000:
            errors.append("Block registry sizes below safety thresholds: " + str(counts))

    jigsaw = output / "data" / "jigsaw_structure_data_packet.bin"
    jigsaw_captured = jigsaw.is_file() and jigsaw.stat().st_size > 0
    if jigsaw_captured:
        data = jigsaw.read_bytes()
        files[jigsaw.name] = {
            "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()
        }
    else:
        print("::notice::Jigsaw packet 313 unavailable without a connected client; 11 standard files still export.")

    if not started:
        errors.append("Server startup not confirmed")
    if not exported:
        errors.append("exportvanilla completion not confirmed")
    report = {
        "timestamp_utc": datetime.now(timezone.utc).isoformat(),
        "repository": os.getenv("GITHUB_REPOSITORY"),
        "commit": os.getenv("GITHUB_SHA"),
        "workflow_run_id": os.getenv("GITHUB_RUN_ID"),
        "bds_version": version,
        "server_started": started,
        "export_completed": exported,
        "jigsaw_packet_captured": jigsaw_captured,
        "jigsaw_format": "bedrock-framed-network-packet-not-nbt" if jigsaw_captured else None,
        "counts": counts, "files": files, "errors": errors,
        "status": "failed" if errors else "success",
    }
    (output / "manifest.json").write_text(
        json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8"
    )
    print("[export] " + json.dumps(report, indent=2, ensure_ascii=False), flush=True)
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
