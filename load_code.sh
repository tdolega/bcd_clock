#!/usr/bin/env bash
set -euo pipefail

repo_root="$(git rev-parse --show-toplevel)"
src="$repo_root/everything.md"

if [[ ! -f "$src" ]]; then
  echo "Missing $src" >&2
  exit 1
fi

python3 - "$repo_root" "$src" <<'PY'
import os
import sys

repo_root = os.path.abspath(sys.argv[1])
src = sys.argv[2]

with open(src, "r", encoding="utf-8") as f:
    lines = f.readlines()

idx = 0
while idx < len(lines):
    line = lines[idx]
    if not line.startswith("## "):
        idx += 1
        continue

    rel_path = line[3:].strip()
    if not rel_path:
        idx += 1
        continue

    idx += 1
    while idx < len(lines) and not lines[idx].startswith("```"):
        idx += 1
    if idx >= len(lines):
        break

    idx += 1
    buf = []
    while idx < len(lines) and not lines[idx].startswith("```"):
        buf.append(lines[idx])
        idx += 1

    if buf and buf[-1] in ("\n", "\r\n"):
        buf.pop()

    out_path = os.path.abspath(os.path.join(repo_root, rel_path))
    if not (out_path == repo_root or out_path.startswith(repo_root + os.sep)):
        raise RuntimeError(f"Refusing to write outside repo: {rel_path}")

    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    with open(out_path, "w", encoding="utf-8", newline="") as out:
        out.writelines(buf)

    idx += 1
PY
